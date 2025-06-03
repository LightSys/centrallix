#David Hopkins June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.common.exceptions import TimeoutException, ElementClickInterceptedException
from datetime import datetime
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
from selenium.webdriver.common.keys import Keys

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
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/html_test.app"
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

        # Test 1 Check for the presence of the HTML element/HTML tags/ HTML content
        html_element = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.CSS_SELECTOR, "html"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - HTML element found: {html_element.tag_name}")
        time.sleep(1)  # Pause to observe HTML element

        # Test 2 Print the content length (characters in the HTML)
        html_content = html_element.get_attribute('innerHTML')
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - HTML content length: {len(html_content)} characters")
        time.sleep(1)  # Pause to observe HTML content

        # Test 3 Extract and print the <title> and first <h1> (body header) from the HTML
        title = driver.title
        try:
            body_header = driver.find_element(By.TAG_NAME, "h1").text
        except Exception:
            body_header = "(No <h1> found)"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Title: {title}")
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Body header (h1): {body_header}")

        # Test 4 Try to input text into a text field
        try:
            text_field = WebDriverWait(driver, 10).until(
                EC.presence_of_element_located((By.CSS_SELECTOR, "input[type='text']"))
            )
            text_field.click()
            text_field.send_keys("LightSys Technology Services")
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Text input successful.")
            time.sleep(3)  # Pause to observe text input
        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Text input field not found.")
        
        # Test 5 Check for the response to the text input (greetingmessage )
        try:
            response_element = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.ID, "greetingMessage"))
            )
            response_text = response_element.text
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Response text: {response_text}")
            time.sleep(1)  # Pause to observe response text
        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Response element not found.")

        # Test 6 Clear the text field 
        try:
            # Manually clear the text field by sending backspaces
            current_value = text_field.get_attribute('value')
            # Click at the end of the text field (move cursor to end)
            text_field.click()
            current_value = text_field.get_attribute('value')
            if current_value:
                # Move cursor to end using END key
                text_field.send_keys(Keys.END)
            for _ in range(len(current_value)):
                text_field.send_keys('\b')
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Text field cleared manually with backspaces.")
            time.sleep(1)  # Pause to observe text field cleared
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error clearing text field: {str(e)}")
        
        # Test 7 Check for the response to the text input (greetingmessage) it is supposed to be empty now 
        try:
            response_element = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.ID, "greetingMessage"))
            )
            response_text = response_element.text
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Response text: {response_text}" if response_text else "Response text is empty.")
            time.sleep(1)  # Pause to observe response text
        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Response element not found.")


    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete, keeping browser open for observation.")
        time.sleep(2)
        driver.quit()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()
