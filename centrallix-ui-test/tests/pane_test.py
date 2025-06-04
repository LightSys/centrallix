# David Hopkins June 2025
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

    test_url = config["url"] + "/tests/ui/pane_test.app"
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

        #There are 5 styles of pane in this test: lowered, raised, flat, transparent, and bordered.

        #Check that there is 5 panes on the page. Doesn't matter what ind they are. 
        #It is under pgmsg and has dynamic div id like pn...main etc. 

        panes = driver.find_elements(By.CSS_SELECTOR, "div[id^='pn']")
        if len(panes) != 5:
            raise Exception(f"Expected 5 panes, found {len(panes)}.")
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found {len(panes)} panes.")
        time.sleep(2)  # Pause to observe panes

    except Exception as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test failed: {str(e)}")

    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete, keeping browser open for observation.")
        time.sleep(2)
        driver.quit()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()