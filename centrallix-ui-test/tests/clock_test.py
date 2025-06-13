# Minsik Lee May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

"""Notes"""

""" Module allowing web testing using pure Selenium """

import toml
import time
import sys
from selenium import webdriver
from selenium.webdriver import ActionChains
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager
from test_reporter import TestReporter
reporter = TestReporter("Clock")

def create_driver(test_url) -> webdriver.Chrome:
    """Create and return a configured Chrome WebDriver."""
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_options.add_argument('--ignore-certificate-errors')  # Skip Certificatgite/by pass it 

    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)

    # Wait until the page has fully loaded
    WebDriverWait(driver, 10).until(
        lambda d: d.execute_script("return document.readyState") == "complete"
    )

    return driver

def get_label_info(label):
    """Return status of the label."""
    try:
        label_text = label.find_element(By.TAG_NAME, "span").text
        # label_text = driver.find_element(By.XPATH, "//div[contains(@id, 'lbl')]//span").text
        return(label_text)
    except Exception as e:
        print(f"Error retrieving label properties: {e}")

def run_test():
    """Run the check box test."""
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print("Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/clock_test.app"
    driver = create_driver(test_url)

    try:
        # Wait for label to appear
        WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located((By.XPATH, "//div[contains(@id, 'lbl')]"))
        )

        # Get elements
        label_widget_name = "clock_label"
        label_elem = driver.execute_script(f"return wgtrFind('{label_widget_name}')")
        clock_widget_name = "clock"
        clock_elem = driver.execute_script(f"return wgtrFind('{clock_widget_name}')")

        # Test 1 = Hover behavior test
        reporter.add_test(1, "Hover behavior test")
        check1_1 = "mouse over event"
        reporter.record_check(1, check1_1, False)        # Initialize check as False
        ActionChains(driver).move_to_element(clock_elem).perform()

        for _ in range(10):
            label = get_label_info(label_elem)
            if label == "Mouse Over":
              reporter.record_check(1, check1_1, True)
              break
            time.sleep(0.5)
        
        check1_2 = "mouse move event"
        reporter.record_check(1, check1_2, False)
        ActionChains(driver).move_to_element(clock_elem).perform()
        if get_label_info(label_elem) == "Mouse Move":
            reporter.record_check(1, check1_2, True)
            
        # Test 2 = Click behavior test
        reporter.add_test(2, "Click behavior test")
        check2_1 = "mouse down event"
        reporter.record_check(2, check2_1, False)
        ActionChains(driver).click_and_hold(clock_elem).perform()
        if get_label_info(label_elem) == "Mouse Down":
            reporter.record_check(2, check2_1, True)

        check2_2 = "mouse up event"
        ActionChains(driver).release().perform()
        reporter.record_check(2, check2_2, False)
        if get_label_info(label_elem) == "Mouse Up":
            reporter.record_check(2, check2_2, True)

    finally:
        # Cleanup
        result = reporter.print_report()
        time.sleep(5)
        driver.quit()
        sys.exit(0) if result else sys.exit(1)


if __name__ == "__main__":
    run_test()  