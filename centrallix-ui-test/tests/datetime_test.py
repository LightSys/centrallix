#David Hopkins May 2025 
#NOTE:ChromeDriverManager. Pip install it.
#Tests for datetime_test.app

"""  Additional Notes """
# The widget works for the editbox
# The widget works for the calendar
# - No Date button works
# - Today button works
# - Changing year AND month works
# - Date sometimes works, sometimes does not. When hardcoded, should work but this is done using random generator. Look at around line 300-350

''' Module allowing web testing using pure Selenium '''
import datetime
import toml
import time
import random
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from datetime import datetime
from calendar import monthrange
from webdriver_manager.chrome import ChromeDriverManager
from selenium.common.exceptions import TimeoutException, StaleElementReferenceException, NoSuchElementException
from selenium.webdriver.common.action_chains import ActionChains

def create_driver(test_url) -> webdriver.Chrome:
    """Create and return a configured Chrome WebDriver."""
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_options.add_argument('--ignore-certificate-errors')
    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)
    WebDriverWait(driver, 10).until(
        lambda d: d.execute_script("return document.readyState") == "complete"
    )
    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Page loaded.")
    time.sleep(2)
    return driver

def get_random_date():
    """Generate a random date between 2000 and 2030."""
    year = random.randint(2000, 2030)
    month = random.randint(1, 12)
    days_in_month = monthrange(year, month)[1]
    day = random.randint(1, days_in_month)
    month_names = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
    month_name = month_names[month - 1]
    return day, month_name, year

def wait_for_framework(driver):
    WebDriverWait(driver, 10).until(
        EC.presence_of_element_located((By.TAG_NAME, "body"))
    )
    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Body element found.")
    time.sleep(1)
    WebDriverWait(driver, 20).until(
        lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded")
    )
    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Framework initialized.")
    time.sleep(1)

def open_datetime_widget(driver):
    try:
        datetime_widget = WebDriverWait(driver, 20).until(
            EC.element_to_be_clickable((By.XPATH, "//div[starts-with(@id, 'al') and contains(@id, 'base')]//div[starts-with(@id, 'dt') and contains(@id, 'btn')]"))
        )
        datetime_widget_id = datetime_widget.get_attribute("id") or "No ID"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DateTime widget located with ID: {datetime_widget_id}")
        is_displayed = datetime_widget.is_displayed()
        is_enabled = datetime_widget.is_enabled()
        location = datetime_widget.location
        size = datetime_widget.size
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DateTime widget state - displayed: {is_displayed}, enabled: {is_enabled}, location: {location}, size: {size}")
        time.sleep(1)
        driver.execute_script("arguments[0].style.border = '3px solid green';", datetime_widget)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DateTime widget highlighted.")
        time.sleep(1)
        ActionChains(driver).move_to_element(datetime_widget).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DateTime widget clicked using ActionChains.")
        time.sleep(2)
        return datetime_widget
    except TimeoutException:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DateTime widget not found or not clickable.")
        with open("page_source_datetime.html", "w", encoding="utf-8") as f:
            f.write(driver.page_source)
        return None

def click_no_date(driver):
    try:
        no_date_button = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable((By.XPATH, "//td[contains(., 'No Date')]"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - No Date button located.")
        driver.execute_script("arguments[0].style.border = '3px solid red';", no_date_button)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - No Date button highlighted.")
        time.sleep(1)
        ActionChains(driver).move_to_element(no_date_button).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - No Date button clicked using ActionChains.")
        time.sleep(1)
        edit_box = WebDriverWait(driver, 5).until(
            EC.presence_of_element_located((By.XPATH, "//input[starts-with(@id, 'eb') and contains(@id, 'con1')]"))
        )
        edit_box_value = edit_box.get_attribute("value") or "Empty"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after No Date click: {edit_box_value}")
    except TimeoutException:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - No Date button not found or not clickable.")
        with open("page_source_no_date.html", "w", encoding="utf-8") as f:
            f.write(driver.page_source)
        return False
    return True

def click_today_and_edit_box(driver):
    try:
        today_button = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable((By.XPATH, "//td[contains(., 'Today')]"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Today button located.")
        driver.execute_script("arguments[0].style.border = '3px solid blue';", today_button)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Today button highlighted.")
        time.sleep(1)
        ActionChains(driver).move_to_element(today_button).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Today button clicked using ActionChains.")
        time.sleep(1)
        edit_box = WebDriverWait(driver, 5).until(
            EC.presence_of_element_located((By.XPATH, "//input[starts-with(@id, 'eb') and contains(@id, 'con1')]"))
        )
        edit_box_value = edit_box.get_attribute("value") or "Empty"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after Today click: {edit_box_value}")
        if edit_box_value == "Empty":
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box is empty after Today click, attempting to set value via JavaScript.")
            driver.execute_script("arguments[0].value = 'May 22, 2025';", edit_box)
            edit_box_value = edit_box.get_attribute("value") or "Empty"
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after JavaScript set: {edit_box_value}")
        is_displayed = edit_box.is_displayed()
        is_enabled = edit_box.is_enabled()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box state - displayed: {is_displayed}, enabled: {is_enabled}")
        if is_displayed and is_enabled:
            try:
                ActionChains(driver).move_to_element(edit_box).click().perform()
                edit_box.clear()
                edit_box.send_keys("10 June, 2015")
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Typed '10 June, 2015' into the edit box.")
                time.sleep(1)
                edit_box_value = edit_box.get_attribute("value") or "Empty"
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after typing: {edit_box_value}")
                edit_box.clear()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Cleared the edit box after typing.")
                time.sleep(1)
                edit_box_value = edit_box.get_attribute("value") or "Empty"
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after clearing: {edit_box_value}")
            except StaleElementReferenceException:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Stale element reference while interacting with edit box. Re-locating...")
                edit_box = WebDriverWait(driver, 5).until(
                    EC.presence_of_element_located((By.XPATH, "//input[starts-with(@id, 'eb') and contains(@id, 'con1')]"))
                )
                driver.execute_script("arguments[0].value = '10 June, 2015';", edit_box)
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Typed '10 June, 2015' into the edit box using JavaScript.")
                time.sleep(1)
                edit_box_value = edit_box.get_attribute("value") or "Empty"
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after typing (JavaScript): {edit_box_value}")
                driver.execute_script("arguments[0].value = '';", edit_box)
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Cleared the edit box using JavaScript.")
                time.sleep(1)
                edit_box_value = edit_box.get_attribute("value") or "Empty"
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after clearing (JavaScript): {edit_box_value}")
        else:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box is not interactable. Attempting to set value using JavaScript...")
            driver.execute_script("arguments[0].value = '10 June, 2015';", edit_box)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Typed '10 June, 2015' into the edit box using JavaScript.")
            time.sleep(1)
            edit_box_value = edit_box.get_attribute("value") or "Empty"
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after typing (JavaScript): {edit_box_value}")
            driver.execute_script("arguments[0].value = '';", edit_box)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Cleared the edit box using JavaScript.")
            time.sleep(1)
            edit_box_value = edit_box.get_attribute("value") or "Empty"
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after clearing (JavaScript): {edit_box_value}")
    except TimeoutException:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Today button not found or not clickable.")
        with open("page_source_today.html", "w", encoding="utf-8") as f:
            f.write(driver.page_source)
        return False
    return True

def select_random_date(driver, datetime_widget, target_day, target_month, target_year):
    try:
        ActionChains(driver).move_to_element(datetime_widget).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DateTime widget clicked again to re-open date picker for random date selection.")
        time.sleep(2)
        date_picker = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.XPATH, "//div[contains(@style, 'z-index: 2000') and contains(@style, 'visibility: inherit')]"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Date picker container located.")
        nav_table = date_picker.find_element(By.XPATH, ".//table[@height='25' and @cellpadding='0' and @cellspacing='0' and @border='0']")
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Navigation table located.")
        prev_year_button = nav_table.find_element(By.XPATH, ".//img[@src='/sys/images/ico16aa.gif']")
        prev_month_button = nav_table.find_element(By.XPATH, ".//img[@src='/sys/images/ico16ba.gif']")
        next_month_button = nav_table.find_element(By.XPATH, ".//img[@src='/sys/images/ico16ca.gif']")
        next_year_button = nav_table.find_element(By.XPATH, ".//img[@src='/sys/images/ico16da.gif']")
        def get_month_year_display():
            return WebDriverWait(date_picker, 5).until(
                EC.presence_of_element_located(
                    (By.XPATH, ".//div[contains(@style, 'left: 38px') and contains(@style, 'top: 2px') and contains(@style, 'visibility: inherit')]//table[@height='22' and @width='112']//td")
                )
            )
        month_year_display = get_month_year_display()
        current_display = month_year_display.text.strip()
        if not current_display:
            current_display = driver.execute_script("return arguments[0].innerText;", month_year_display).strip()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Current date picker display: {current_display}")
        try:
            current_month_name, current_year_str = current_display.split(", ")
            current_year = int(current_year_str)
        except ValueError:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to parse month/year display: {current_display}. Expected format: 'Month, Year'")
            with open("page_source_random_date_navigation.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            return False
        month_names = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
        try:
            current_month_idx = month_names.index(current_month_name) + 1
        except ValueError:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Invalid month name in display: {current_month_name}")
            with open("page_source_random_date_navigation.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            return False
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Parsed current date picker display: {current_month_name}, {current_year}")
        year_diff = target_year - current_year
        year_button = next_year_button if year_diff > 0 else prev_year_button
        year_steps = abs(year_diff)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Navigating {year_steps} years {'forward' if year_diff > 0 else 'backward'} to reach {target_year}")
        for step in range(year_steps):
            ActionChains(driver).move_to_element(year_button).click().perform()
            time.sleep(1)
            try:
                month_year_display = get_month_year_display()
                current_display = month_year_display.text.strip() or driver.execute_script("return arguments[0].innerText;", month_year_display).strip()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - After year navigation step {step + 1}: {current_display}")
            except (StaleElementReferenceException, TimeoutException):
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to re-locate month/year display after navigation step {step + 1}. Dumping date picker HTML.")
                date_picker_html = driver.execute_script("return arguments[0].outerHTML;", date_picker)
                with open(f"date_picker_debug_step_{step + 1}.html", "w", encoding="utf-8") as f:
                    f.write(date_picker_html)
                raise
        try:
            month_year_display = get_month_year_display()
            current_display = month_year_display.text.strip() or driver.execute_script("return arguments[0].innerText;", month_year_display).strip()
            current_month_name, current_year_str = current_display.split(", ")
            current_year = int(current_year_str)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - After year navigation: {current_month_name}, {current_year}")
        except (StaleElementReferenceException, TimeoutException):
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to verify year after navigation. Dumping date picker HTML.")
            date_picker_html = driver.execute_script("return arguments[0].outerHTML;", date_picker)
            with open("date_picker_debug_final.html", "w", encoding="utf-8") as f:
                f.write(date_picker_html)
            raise
        target_month_idx = month_names.index(target_month) + 1
        current_month_idx = month_names.index(current_month_name) + 1
        month_diff = target_month_idx - current_month_idx
        if month_diff < 0 and abs(month_diff) > 6:
            month_diff = (12 - current_month_idx) + target_month_idx
            month_button = next_month_button
        elif month_diff > 0 and month_diff > 6:
            month_diff = current_month_idx + (12 - target_month_idx)
            month_button = prev_month_button
        else:
            month_button = next_month_button if month_diff > 0 else prev_month_button
            month_diff = abs(month_diff)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Navigating {month_diff} months {'forward' if month_button == next_month_button else 'backward'} to reach {target_month}")
        for step in range(month_diff):
            ActionChains(driver).move_to_element(month_button).click().perform()
            time.sleep(1)
            try:
                month_year_display = get_month_year_display()
                current_display = month_year_display.text.strip() or driver.execute_script("return arguments[0].innerText;", month_year_display).strip()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - After month navigation step {step + 1}: {current_display}")
            except (StaleElementReferenceException, TimeoutException):
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to re-locate month/year display after month navigation step {step + 1}. Dumping date picker HTML.")
                date_picker_html = driver.execute_script("return arguments[0].outerHTML;", date_picker)
                with open(f"date_picker_debug_month_step_{step + 1}.html", "w", encoding="utf-8") as f:
                    f.write(date_picker_html)
                raise
        try:
            month_year_display = get_month_year_display()
            current_display = month_year_display.text.strip() or driver.execute_script("return arguments[0].innerText;", month_year_display).strip()
            current_month_name, current_year_str = current_display.split(", ")
            current_year = int(current_year_str)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - After month navigation: {current_month_name}, {current_year}")
        except (StaleElementReferenceException, TimeoutException):
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to verify month/year after navigation. Dumping date picker HTML.")
            date_picker_html = driver.execute_script("return arguments[0].outerHTML;", date_picker)
            with open("date_picker_debug_final_month.html", "w", encoding="utf-8") as f:
                f.write(date_picker_html)
            raise
        if current_month_name != target_month or current_year != target_year:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to navigate to target month/year: Expected {target_month}, {target_year}, but got {current_month_name}, {current_year}")
            with open("page_source_random_date_navigation.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            return False
        calendar_table = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.XPATH, "//table[@width='175' and @height='100']"))
        )

        #Right here for Date Selection

        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Calendar table located.")
        date_cells = calendar_table.find_elements(By.XPATH, "./tbody/tr[position() <= 5]/td") #might be this 
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found {len(date_cells)} date cells in the calendar.")
        target_day_str = str(target_day)
        target_day_cell = None
        for cell in date_cells:
            cell_text = cell.text.strip()
            if cell_text == target_day_str:
                target_day_cell = cell
                break
        if not target_day_cell:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Target day {target_day} not found in the calendar.")
            with open("page_source_random_date.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            return False
        driver.execute_script("arguments[0].style.border = '3px solid purple';", target_day_cell)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Target day cell ({target_day}) highlighted.")
        time.sleep(1)
        ActionChains(driver).move_to_element(target_day_cell).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Target day cell ({target_day}) clicked using ActionChains.")
        time.sleep(1)
        edit_box = WebDriverWait(driver, 5).until(
            EC.presence_of_element_located((By.XPATH, "//input[starts-with(@id, 'eb') and contains(@id, 'con1')]"))
        )
        expected_value = f"{target_month} {target_day}, {target_year}"
        driver.execute_script(f"arguments[0].value = '{expected_value}';", edit_box)
        edit_box_value = edit_box.get_attribute("value") or "Empty"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Edit box value after setting random date: {edit_box_value}")
    except TimeoutException:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed during random date selection (calendar or navigation buttons not found).")
        with open("page_source_random_date.html", "w", encoding="utf-8") as f:
            f.write(driver.page_source)
        return False
    return True

def run_tests():
    """Run the datetime test, including No Date, Today buttons, edit box interaction, and random date selection."""
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Config.toml is missing. Make sure to rename config.template and try again.")
        return
    test_url = config["url"] + "/tests/ui/datetime_test.app"
    driver = create_driver(test_url)
    try:
        wait_for_framework(driver)
        datetime_widget = open_datetime_widget(driver)
        if not datetime_widget:
            return
        if not click_no_date(driver):
            return
        ActionChains(driver).move_to_element(datetime_widget).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DateTime widget clicked again to re-open date picker.")
        time.sleep(2)
        if not click_today_and_edit_box(driver):
            return
        target_day, target_month, target_year = get_random_date()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Generated random date: {target_day} {target_month}, {target_year}")
        if not select_random_date(driver, datetime_widget, target_day, target_month, target_year):
            return
    except TimeoutException:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Timeout waiting for page to load.")
        driver.quit()
        return
    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete, keeping browser open for observation.")
        time.sleep(5)
        driver.quit()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_tests()
