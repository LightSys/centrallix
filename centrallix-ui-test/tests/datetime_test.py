# David Hopkins - (Updated) June 2025
# NOTE: USE ChromeDriverManager. Pip install it.
# Tests for datetime_test.app

import toml
import time
import random
from datetime import datetime
from calendar import monthrange
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import TimeoutException, StaleElementReferenceException, NoSuchElementException
from selenium.webdriver.common.action_chains import ActionChains
from webdriver_manager.chrome import ChromeDriverManager
import sys

class TestBlock:
    """A class to manage a block of test checks and format the output."""
    def __init__(self, number, name):
        self.number = number
        self.name = name
        self.checks = []

    def start(self):
        """Prints the header for this test block."""
        print(f"TEST {self.number} = {self.name}")

    def add_check(self, description, passed: bool):
        """Adds a check to the block and prints its immediate status."""
        self.checks.append(passed)
        status = "PASS" if passed else "FAIL"
        print(f"    Test {description} ... {status}")

    def conclude(self) -> bool:
        """Prints the summary for the block and returns its overall status."""
        passed_count = sum(1 for p in self.checks if p)
        total_count = len(self.checks)
        block_passed = passed_count == total_count and total_count > 0
        status = "PASS" if block_passed else "FAIL"
        print(f"({passed_count}/{total_count}) {status}\n")
        return block_passed

def get_random_date():
    """Generate a random date between 2000 and 2030."""
    year = random.randint(2000, 2030)
    month = random.randint(1, 12)
    days_in_month = monthrange(year, month)[1]
    day = random.randint(1, days_in_month)
    month_names = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
    month_name = month_names[month - 1]
    return day, month_name, year

def run_tests():
    """Run the datetime test, reporting results in a structured format."""
    print("# UI Test coverage: Datetime Widget Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/datetime_test.app"

        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')
        
        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Page and Framework Initialization ---
        init_test = TestBlock(1, "Page and Framework Initialization")
        init_test.start()
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page ready state is complete", True)
        except Exception:
            init_test.add_check("page ready state is complete", False)
        
        try:
            WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.TAG_NAME, "body")))
            init_test.add_check("body element is present", True)
        except Exception:
            init_test.add_check("body element is present", False)

        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework (pg_isloaded) is initialized", True)
        except Exception:
            init_test.add_check("framework (pg_isloaded) is initialized", False)
        
        all_blocks_passed.append(init_test.conclude())
        
        # --- TEST 2: Datetime Widget Features ---
        widget_test = TestBlock(2, "Datetime Widget Features")
        widget_test.start()
        
        datetime_widget = None
        try:
            xpath = "//div[starts-with(@id, 'al') and contains(@id, 'base')]//div[starts-with(@id, 'dt') and contains(@id, 'btn')]"
            datetime_widget = WebDriverWait(driver, 20).until(EC.element_to_be_clickable((By.XPATH, xpath)))
            driver.execute_script("arguments[0].style.border = '3px solid green';", datetime_widget)
            time.sleep(1)
            ActionChains(driver).move_to_element(datetime_widget).click().perform()
            time.sleep(2)
            widget_test.add_check("widget can be located and opened", True)
        except Exception:
            widget_test.add_check("widget can be located and opened", False)

        if datetime_widget:
            try:
                no_date_button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//td[contains(., 'No Date')]")))
                driver.execute_script("arguments[0].style.border = '3px solid red';", no_date_button)
                time.sleep(1)
                ActionChains(driver).move_to_element(no_date_button).click().perform()
                time.sleep(1)
                widget_test.add_check("'No Date' button is clickable", True)
            except Exception:
                widget_test.add_check("'No Date' button is clickable", False)
            
            try:
                ActionChains(driver).move_to_element(datetime_widget).click().perform() # Re-open
                time.sleep(2)
                today_button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//td[contains(., 'Today')]")))
                driver.execute_script("arguments[0].style.border = '3px solid blue';", today_button)
                time.sleep(1)
                ActionChains(driver).move_to_element(today_button).click().perform()
                time.sleep(1)
                widget_test.add_check("'Today' button is clickable", True)
            except Exception:
                widget_test.add_check("'Today' button is clickable", False)
        else:
            widget_test.add_check("'No Date' button is clickable (SKIPPED)", False)
            widget_test.add_check("'Today' button is clickable (SKIPPED)", False)

        all_blocks_passed.append(widget_test.conclude())
        
        # --- TEST 3: Edit Box Direct Input ---
        edit_box_test = TestBlock(3, "Edit Box Direct Input")
        edit_box_test.start()
        try:
            edit_box_xpath = "//input[starts-with(@id, 'eb') and contains(@id, 'con1')]"
            edit_box = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.XPATH, edit_box_xpath)))
            ActionChains(driver).move_to_element(edit_box).click().perform()
            edit_box.clear()
            edit_box.send_keys("10 June, 2015")
            time.sleep(5)
            edit_box_test.add_check("typing directly into the edit box", True)
            
            edit_box.clear()
            time.sleep(3)
            edit_box_test.add_check("clearing the edit box", True)
        except Exception:
            edit_box_test.add_check("typing directly into the edit box", False)
            edit_box_test.add_check("clearing the edit box", False)

        all_blocks_passed.append(edit_box_test.conclude())

        # --- TEST 4: Calendar Navigation and Selection ---
        calendar_test = TestBlock(4, "Calendar Navigation and Selection")
        calendar_test.start()
        if datetime_widget:
            try:
                target_day, target_month, target_year = get_random_date()
                
                # Re-open the date picker
                ActionChains(driver).move_to_element(datetime_widget).click().perform()
                time.sleep(2)
                
                # Locate navigation elements
                date_picker_xpath = "//div[contains(@style, 'z-index: 2000') and contains(@style, 'visibility: inherit')]"
                date_picker = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.XPATH, date_picker_xpath)))

                prev_year_button = date_picker.find_element(By.XPATH, ".//img[@src='/sys/images/ico16aa.gif']")
                prev_month_button = date_picker.find_element(By.XPATH, ".//img[@src='/sys/images/ico16ba.gif']")
                next_month_button = date_picker.find_element(By.XPATH, ".//img[@src='/sys/images/ico16ca.gif']")
                next_year_button = date_picker.find_element(By.XPATH, ".//img[@src='/sys/images/ico16da.gif']")

                # Get current month/year display
                display_xpath = ".//div[contains(@style, 'visibility: inherit')]//table[@height='22' and @width='112']//td"
                month_year_display = WebDriverWait(date_picker, 5).until(EC.presence_of_element_located((By.XPATH, display_xpath)))
                current_display = month_year_display.text.strip() or driver.execute_script("return arguments[0].innerText;", month_year_display).strip()
                
                # Parse current display
                current_month_name, current_year_str = current_display.split(", ")
                current_year = int(current_year_str)
                month_names = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
                
                # Navigate years
                year_diff = target_year - current_year
                year_button = next_year_button if year_diff > 0 else prev_year_button
                for _ in range(abs(year_diff)):
                    ActionChains(driver).move_to_element(year_button).click().perform()
                    time.sleep(0.5)

                # Navigate months
                current_month_name_after_nav = WebDriverWait(date_picker, 5).until(EC.presence_of_element_located((By.XPATH, display_xpath))).text.strip().split(", ")[0]
                current_month_idx = month_names.index(current_month_name_after_nav)
                target_month_idx = month_names.index(target_month)
                month_diff = target_month_idx - current_month_idx
                month_button = next_month_button if month_diff > 0 else prev_month_button
                for _ in range(abs(month_diff)):
                    ActionChains(driver).move_to_element(month_button).click().perform()
                    time.sleep(0.5)

                # Select the day
                calendar_table = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.XPATH, "//table[@width='175' and @height='100']")))
                day_cell = calendar_table.find_element(By.XPATH, f".//td[text()='{target_day}']")
                driver.execute_script("arguments[0].style.border = '3px solid purple';", day_cell)
                time.sleep(1)
                ActionChains(driver).move_to_element(day_cell).click().perform()
                time.sleep(1)
                
                calendar_test.add_check(f"selecting random date ({target_month} {target_day}, {target_year})", True)
            except Exception:
                calendar_test.add_check("selecting random date via calendar navigation", False)
        else:
            calendar_test.add_check("selecting random date via calendar navigation (SKIPPED)", False)

        all_blocks_passed.append(calendar_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 4:
            all_blocks_passed.append(False)
    
    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"Datetime Widget Test {final_status}")
        print("---")
        sys.exit(0 if final_status == "PASS" else 1)

        
        if driver:
            print("Test complete. Browser will close in 5 seconds.")
            time.sleep(5)
            driver.quit()

if __name__ == "__main__":
    run_tests()