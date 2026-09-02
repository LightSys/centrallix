# David Hopkins June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.common.exceptions import TimeoutException, ElementClickInterceptedException
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
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
        print(f"START TEST {self.number}")

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
        print(f"TEST {self.number} = {self.name} ({passed_count}/{total_count}) {status}\n")
        return block_passed

def run_test():
    """Runs the scrollbar test with structured reporting."""
    print("# UI Test coverage: Scrollbar Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/scrollbar_test.app"

        # --- Driver and Page Initialization ---
        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')

        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Page Initialization ---
        init_test = TestBlock(1, "Page Initialization")
        init_test.start()
        
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page loaded successfully", True)
        except Exception:
            init_test.add_check("page loaded successfully", False)
        
        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework is initialized (pg_isloaded)", True)
        except Exception:
            init_test.add_check("framework is initialized (pg_isloaded)", False)
        
        all_blocks_passed.append(init_test.conclude())

        # --- TEST 2: Scrollbar Interaction ---
        scrollbar_test = TestBlock(2, "Scrollbar Interaction")
        scrollbar_test.start()
        
        # 1. Click the forward button
        try:
            forward_locators = [
                (By.XPATH, "//img[@src='/sys/images/ico18b.gif']"),
                (By.XPATH, "//img[@name='d']"),
                (By.XPATH, "//div[@id='sb17pane']//table//td[3]/img"),
            ]
            forward_button = None
            for locator in forward_locators:
                try:
                    forward_button = WebDriverWait(driver, 2).until(EC.element_to_be_clickable(locator))
                    if forward_button: break
                except TimeoutException: continue
            
            assert forward_button is not None, "Forward button not found"
            driver.execute_script("arguments[0].scrollIntoView({block: 'center'});", forward_button)
            time.sleep(3)
            forward_button.click()
            scrollbar_test.add_check("clicking the forward arrow button", True)
        except ElementClickInterceptedException:
            driver.execute_script("arguments[0].click();", forward_button)
            scrollbar_test.add_check("clicking the forward arrow button (using JS)", True)
        except Exception as e:
            scrollbar_test.add_check("clicking the forward arrow button", False)
            print(f"    (Error) {e}")

        time.sleep(1)

        # 2. Click the backward button
        try:
            backward_locators = [
                (By.XPATH, "//img[@src='/sys/images/ico19b.gif']"),
                (By.XPATH, "//img[@name='u']"),
                (By.XPATH, "//div[@id='sb17pane']//table//td[1]/img"),
            ]
            backward_button = None
            for locator in backward_locators:
                try:
                    backward_button = WebDriverWait(driver, 2).until(EC.element_to_be_clickable(locator))
                    if backward_button: break
                except TimeoutException: continue

            assert backward_button is not None, "Backward button not found"
            driver.execute_script("arguments[0].scrollIntoView({block: 'center'});", backward_button)
            time.sleep(3)
            backward_button.click()
            scrollbar_test.add_check("clicking the backward arrow button", True)
        except ElementClickInterceptedException:
            driver.execute_script("arguments[0].click();", backward_button)
            scrollbar_test.add_check("clicking the backward arrow button (using JS)", True)
        except Exception as e:
            scrollbar_test.add_check("clicking the backward arrow button", False)
            print(f"    (Error) {e}")

        time.sleep(1)

        # 3. Drag the scrollbar thumb
        try:
            thumb_locators = [
                (By.XPATH, "//img[@src='/sys/images/ico14b.gif']"),
                (By.XPATH, "//img[@name='t']"),
                (By.XPATH, "//div[@id='sb17thum']/img"),
            ]
            thumb = None
            for locator in thumb_locators:
                try:
                    thumb = WebDriverWait(driver, 2).until(EC.element_to_be_clickable(locator))
                    if thumb: break
                except TimeoutException: continue
            
            assert thumb is not None, "Scrollbar thumb not found"
            driver.execute_script("arguments[0].scrollIntoView({block: 'center'});", thumb)
            time.sleep(3)
            
            actions = ActionChains(driver)
            actions.click_and_hold(thumb).move_by_offset(62, 0).release().perform()
            scrollbar_test.add_check("dragging the scrollbar thumb", True)
        except Exception as e:
            scrollbar_test.add_check("dragging the scrollbar thumb", False)
            print(f"    (Error) {e}")

        all_blocks_passed.append(scrollbar_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 2:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"Scrollbar Test {final_status}")
        print("---")
        sys.exit(0 if final_status == "PASS" else 1)

        if driver:
            print("Test complete. Browser will close in 2 seconds.")
            time.sleep(2)
            driver.quit()

if __name__ == "__main__":
    run_test()