# Minsik Lee May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

"""Notes"""
# Readonly checkbox is clickable and its status gets updated

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


def get_checkbox_info(driver: webdriver.Chrome, index) -> None:
    """Return clicked status of a checkbox."""
    try:
        checkbox_elements = driver.find_elements(By.XPATH, "//div[contains(@id,'cb')]//img")
        imgsrc = checkbox_elements[index].get_attribute("src")
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
        # Wait for checkbox to appear
        WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located((By.XPATH, "//div[contains(@id,'cb')]"))
        )

        # Initial read
        print(f"Initial Checbox info: {get_checkbox_info(driver, 0)}")
        
        # Click checkbox 3 times
        checkbox_elem = driver.find_element(By.XPATH, "//div[contains(@id,'cb')]")
        for i in range(3):
            ActionChains(driver).click(checkbox_elem).perform()
            print(f"Checkbox status after click: {get_checkbox_info(driver,0)}")

        # Click Readonly Checkbox
        readonly_checkbox_elem = driver.find_element(By.XPATH, "//div[contains(@id,'cb')][last()]")
        print(f"readonly cb status before click: {get_checkbox_info(driver, 2)}")
        ActionChains(driver).click(readonly_checkbox_elem).perform()
        print(f"readonly cb status after click: {get_checkbox_info(driver, 2)}")

    finally:
        # Cleanup
        time.sleep(10)
        driver.quit()


if __name__ == "__main__":
    run_test()