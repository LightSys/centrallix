# Updated by: [David Hopkins] May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

""" Module allowing web testing using pure Selenium """

from selenium.webdriver.common.alert import Alert
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.common.exceptions import TimeoutException
import toml
import time
from datetime import datetime
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager

def create_driver(test_url) -> webdriver.Chrome:
    """Create and return a configured Chrome WebDriver."""
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_options.add_argument('--ignore-certificate-errors')  # Skip SSL errors

    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)

    # Wait until the page has fully loaded
    WebDriverWait(driver, 10).until(
        lambda d: d.execute_script("return document.readyState") == "complete"
    )
    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Page loaded.")
    time.sleep(2)  # Pause to observe page load
    return driver

def run_test():
    """Run the button functionality test slowly for visibility."""
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/textbutton_test.app"
    driver = create_driver(test_url)

    try:
        # Wait for the page to load and framework to initialize
        WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.TAG_NAME, "body"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Body element found.")
        time.sleep(2)  # Pause to observe page

        WebDriverWait(driver, 20).until(
            lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded")
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Framework initialized.")
        time.sleep(2)  # Pause to observe initialization

        # Locate and test Button 1
        try:
            button1 = WebDriverWait(driver, 20).until(
                EC.element_to_be_clickable((By.XPATH, "//div[contains(@class, 'cell') and .//span[text()='Button 1']]"))
            )
            button1_id = button1.get_attribute("id") or "No ID"
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 1 located with ID: {button1_id}")
            time.sleep(2)  # Pause to observe Button 1 location

            # Highlight Button 1
            driver.execute_script("arguments[0].style.border = '3px solid red';", button1)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 1 highlighted.")
            time.sleep(2)  # Pause to see highlight

            # Wait for button to be ready
            WebDriverWait(driver, 20).until(
                lambda d: d.find_element(By.XPATH, "//div[contains(@class, 'cell') and .//span[text()='Button 1']]").value_of_css_property("cursor") == "default"
            )
            is_enabled = button1.is_enabled()
            is_displayed = button1.is_displayed()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 1 enabled: {is_enabled}, displayed: {is_displayed}")
            time.sleep(2)  # Pause to observe state

            if is_enabled and is_displayed:
                button1.click()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 1 clicked.")
            else:
                driver.execute_script("arguments[0].click();", button1)
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 1 clicked via JavaScript.")
            time.sleep(2)  # Pause to observe click

            # Handle the alert
            WebDriverWait(driver, 10).until(EC.alert_is_present())
            alert = Alert(driver)
            alert_text = alert.text
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 1 alert text: {alert_text}")
            time.sleep(2)  # Pause to observe alert
            alert.accept()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 1 alert accepted.")
            time.sleep(2)  # Pause to observe alert closing
        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 1 not found, not clickable, or alert not present.")
            with open("page_source_button1.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            return

        # Locate and test Button 2
        try:
            button2 = WebDriverWait(driver, 20).until(
                EC.element_to_be_clickable((By.XPATH, "//div[contains(@class, 'cell') and .//span[text()='Button 2']]"))
            )
            button2_id = button2.get_attribute("id") or "No ID"
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 2 located with ID: {button2_id}")
            time.sleep(2)  # Pause to observe Button 2 location

            # Highlight Button 2
            driver.execute_script("arguments[0].style.border = '3px solid blue';", button2)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 2 highlighted.")
            time.sleep(2)  # Pause to see highlight

            # Wait for button to be ready
            WebDriverWait(driver, 20).until(
                lambda d: d.find_element(By.XPATH, "//div[contains(@class, 'cell') and .//span[text()='Button 2']]").value_of_css_property("cursor") == "default"
            )
            is_enabled = button2.is_enabled()
            is_displayed = button2.is_displayed()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 2 enabled: {is_enabled}, displayed: {is_displayed}")
            time.sleep(2)  # Pause to observe state

            if is_enabled and is_displayed:
                button2.click()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 2 clicked.")
            else:
                driver.execute_script("arguments[0].click();", button2)
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 2 clicked via JavaScript.")
            time.sleep(2)  # Pause to observe click

            # Handle the alert
            WebDriverWait(driver, 10).until(EC.alert_is_present())
            alert = Alert(driver)
            alert_text = alert.text
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 2 alert text: {alert_text}")
            time.sleep(2)  # Pause to observe alert
            alert.accept()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 2 alert accepted.")
            time.sleep(2)  # Pause to observe alert closing
        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button 2 not found, not clickable, or alert not present.")
            with open("page_source_button2.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            return

    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete, keeping browser open for observation.")
        time.sleep(2) 
        driver.quit()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()