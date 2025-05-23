# Updated by: [David Hopkins] May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

""" Module allowing web testing using pure Selenium """

import toml
import time
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager



def create_driver(test_url) -> webdriver.Chrome:
    """Create and return a configured Chrome WebDriver."""
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_options.add_argument('--ignore-certificate-errors')  # Skip Certificate/by pass it 

    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)

    # Wait until the page has fully loaded
    WebDriverWait(driver, 10).until(
        lambda d: d.execute_script("return document.readyState") == "complete"
    )

    return driver


def print_input_info(driver: webdriver.Chrome) -> None:
    """Print content, value, CSS, and focus status of the last input element."""
    try:
        input_elem = driver.find_element(By.XPATH, "//input[last()]")

        # Print basic properties
        print(f"Content: {input_elem.get_attribute('textContent') or 'None'}")
        print(f"Value: {input_elem.get_attribute('value') or 'None'}")

        # Check if input is focused
        is_focused = driver.execute_script("return document.activeElement === arguments[0];", input_elem)
        print(f"Is focused: {is_focused}")

        # CSS styles
        print(f"Background color: {input_elem.value_of_css_property('background-color')}")
        print(f"Text color: {input_elem.value_of_css_property('color')}\n")

    except Exception as e:
        print(f"Error retrieving input properties: {e}")


def run_test():
    """Run the edit box test."""
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print("Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/editbox_test.app"
    driver = create_driver(test_url)

    try:
        # Wait for input box to appear
        WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.XPATH, "//input[last()]"))
        )

        # Initial read
        print("Initial input state:")
        print_input_info(driver)

        input_elem = driver.find_element(By.XPATH, "//input[last()]")

        # Type into the input
        input_elem.send_keys("From Python")
        print("After sending keys from Python:")
        print_input_info(driver)

        # Click the input
        input_elem.click()
        print("After clicking the input:")
        print_input_info(driver)

        # Inject JS value directly
        driver.execute_script("arguments[0].value = 'Hello, LightSys!';", input_elem)
        input_elem.click()
        print("After setting value from JavaScript:")
        print_input_info(driver)

    finally:
        # Cleanup
        time.sleep(10)
        driver.quit()


if __name__ == "__main__":
    run_test()