# David Hopkins May 2025
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

    test_url = config["url"] + "/tests/ui/scrollbar_test.app"
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

        # 1. Click the forward button that is shown by an arrow
        try:
            # Define locators for the right arrow (forward button)
            forward_locators = [
                (By.XPATH, "//img[@src='/sys/images/ico18b.gif']"),  # By image source
                (By.XPATH, "//img[@name='d']"),  # By name attribute
                (By.XPATH, "//div[@id='sb17pane']//table//td[3]/img"),  # By table position
            ]

            forward_button = None
            for locator in forward_locators:
                try:
                    forward_button = WebDriverWait(driver, 10).until(
                        EC.element_to_be_clickable(locator)
                    )
                    if forward_button:
                        break
                except TimeoutException:
                    continue

            if not forward_button:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Forward button not found with any locator.")
                print("Page HTML:")
                print(driver.page_source)
                return

            # Scroll to the element to ensure it's in view
            driver.execute_script("arguments[0].scrollIntoView({block: 'center'});", forward_button)
            time.sleep(0.5)  # Brief pause after scrolling

            # Attempt to click the forward button
            try:
                forward_button.click()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Forward button clicked successfully.")
            except ElementClickInterceptedException:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Forward button click intercepted, attempting JavaScript click.")
                driver.execute_script("arguments[0].click();", forward_button)
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Forward button clicked via JavaScript.")

            time.sleep(2)  # Pause to observe click

        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error clicking forward button: {str(e)}")
            print("Page HTML:")
            print(driver.page_source)
            return

        # 2. Click the backward button that is shown by an arrow
        try:
            # Define locators for the left arrow (backward button)
            backward_locators = [
                (By.XPATH, "//img[@src='/sys/images/ico19b.gif']"),  # By image source
                (By.XPATH, "//img[@name='u']"),  # By name attribute
                (By.XPATH, "//div[@id='sb17pane']//table//td[1]/img"),  # By table position
            ]

            backward_button = None
            for locator in backward_locators:
                try:
                    backward_button = WebDriverWait(driver, 10).until(
                        EC.element_to_be_clickable(locator)
                    )
                    if backward_button:
                        break
                except TimeoutException:
                    continue

            if not backward_button:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Backward button not found with any locator.")
                print("Page HTML:")
                print(driver.page_source)
                return

            # Scroll to the element to ensure it's in view
            driver.execute_script("arguments[0].scrollIntoView({block: 'center'});", backward_button)
            time.sleep(0.5)  # Brief pause after scrolling

            # Attempt to click the backward button
            try:
                backward_button.click()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Backward button clicked successfully.")
            except ElementClickInterceptedException:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Backward button click intercepted, attempting JavaScript click.")
                driver.execute_script("arguments[0].click();", backward_button)
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Backward button clicked via JavaScript.")

            time.sleep(2)  # Pause to observe click

        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error clicking backward button: {str(e)}")
            print("Page HTML:")
            print(driver.page_source)
            return

        # 3. Drag the scrollbar thumb freely
        try:
            # Define locators for the scrollbar thumb
            thumb_locators = [
                (By.XPATH, "//img[@src='/sys/images/ico14b.gif']"),  # By image source
                (By.XPATH, "//img[@name='t']"),  # By name attribute
                (By.XPATH, "//div[@id='sb17thum']/img"),  # By parent div
            ]

            thumb = None
            for locator in thumb_locators:
                try:
                    thumb = WebDriverWait(driver, 10).until(
                        EC.element_to_be_clickable(locator)
                    )
                    if thumb:
                        break
                except TimeoutException:
                    continue

            if not thumb:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Scrollbar thumb not found with any locator.")
                print("Page HTML:")
                print(driver.page_source)
                return

            # Scroll to the element to ensure it's in view
            driver.execute_script("arguments[0].scrollIntoView({block: 'center'});", thumb)
            time.sleep(0.5)  # Brief pause after scrolling

            # Drag the thumb by 62ixels to the right (adjustable)
            drag_offset_x = 62 # Positive for right, negative for left
            try:
                actions = ActionChains(driver)
                actions.click_and_hold(thumb).move_by_offset(drag_offset_x, 0).release().perform()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Scrollbar thumb dragged {drag_offset_x}px successfully.")
            except Exception as e:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error dragging scrollbar thumb: {str(e)}")
            time.sleep(2)  # Pause to observe drag

        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error dragging scrollbar thumb: {str(e)}")
            print("Page HTML:")
            print(driver.page_source)
            return

    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete, keeping browser open for observation.")
        time.sleep(2)
        driver.quit()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()