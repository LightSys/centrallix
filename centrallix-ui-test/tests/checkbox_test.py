# Minsik Lee May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

"""Notes"""
# Readonly checkbox is clickable and its status gets updated

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
reporter = TestReporter("Checkbox")

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


def get_checkbox_info(driver: webdriver.Chrome, cb) -> None:
    """Return clicked status of a checkbox."""
    try:
        if cb is None:
            return "ERROR (get_checkbox_info): checkbox not found"
        imgsrc = cb.find_element(By.TAG_NAME, "img").get_attribute("src")
        if "un" in imgsrc:
            return("unchecked")
        elif "checked" in imgsrc:
            return("checked")
        elif "null" in imgsrc:
            return("null")

    except Exception as e:
        print(f"Error retrieving input properties: {e}")


def run_test():
    """Run the check box test."""
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print("Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/checkbox_test.app"
    driver = create_driver(test_url)

    try:
        # Get test elements
        cb_names = ["checkbox1", "checkbox2", "checkbox3"]
        checkboxes = []
        for cb_name in cb_names:
            checkboxes.append(driver.execute_script(f"return wgtrFind('{cb_name}')")) 
        
        for cb in checkboxes:
            if cb is None:
                raise ValueError("Checkbox element not found")

        # TEST 1 = Click behavior test
        reporter.add_test(1, "Click behavior test")
        #   Test clicked status change
        reporter.record_check(1, "checkbox state change", True)
        for checkbox in checkboxes:
            case_order = ["unchecked", "checked", "null"]
            initial_index = case_order.index(get_checkbox_info(driver, checkbox))
            for _ in range(3):
                ActionChains(driver).click(checkbox).perform()
                if get_checkbox_info(driver, checkbox) != case_order[(initial_index + 1) % 3]:
                    reporter.record_check(1, "checkbox state change", False)
                initial_index += 1
                
    finally:
        # Cleanup
        result = reporter.print_report()
        time.sleep(5)
        driver.quit()
        sys.exit(0) if result else sys.exit(1)


if __name__ == "__main__":
    run_test()