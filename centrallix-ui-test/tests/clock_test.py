# Minsik Lee May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

"""Notes"""

""" Module allowing web testing using pure Selenium """

import toml
import time
from selenium import webdriver
from selenium.webdriver import ActionChains
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
    chrome_options.add_argument('--ignore-certificate-errors')  # Skip Certificatgite/by pass it 

    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)

    # Wait until the page has fully loaded
    WebDriverWait(driver, 10).until(
        lambda d: d.execute_script("return document.readyState") == "complete"
    )

    return driver

def get_label_info(driver: webdriver.Chrome):
    """Return status of the label."""
    try:
        label_element = driver.find_element(By.XPATH, "//div[contains(@id, 'lbl')]//span").text
        return(label_element)
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

        # Initial read
        print(f"Initial label text: {get_label_info(driver)}\n")

        # Get clock element
        clock_element = driver.find_element(By.XPATH, "//div[contains(@id, 'cl')]")

        # Test Mouse Over
        ActionChains(driver).move_to_element(clock_element).perform()
        print("Testing mouse over ...")

        passed = False
        for _ in range(10):
            label = get_label_info(driver)
            if label == "Mouse Over":
              print("Mouse Over: PASS")
              passed = True
              break
            time.sleep(0.5)

        if not passed:
          raise ValueError("Mouse Over: FAIL")
        
        # Test Mouse Move
        ActionChains(driver).move_to_element(clock_element).perform()
        if get_label_info(driver) == "Mouse Move":
            print("Mouse Move: PASS")
        else:
            raise ValueError("Mouse Move: FAIL")
            
        # Test Mouse Down
        ActionChains(driver).click_and_hold(clock_element).perform()
        if get_label_info(driver) == "Mouse Down":
            print("Mouse Down: PASS")
        else:
            raise ValueError("Mouse Down: FAIL")

        # Test Mouse Up
        ActionChains(driver).release().perform()
        if get_label_info(driver) == "Mouse Up":
            print("Mouse Up: PASS")
        else:
            raise ValueError("Mouse Up: FAIL")
        print("Test All Passed\nExiting ...")

    finally:
        # Cleanup
        time.sleep(10)
        driver.quit()


if __name__ == "__main__":
    run_test()  