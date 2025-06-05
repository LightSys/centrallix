# David Hopkns June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
import random
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains # Not used in this version, can be removed if not needed elsewhere
from selenium.common.exceptions import TimeoutException, ElementClickInterceptedException
from datetime import datetime
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager

def create_driver(test_url) -> webdriver.Chrome:
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_options.add_argument('--ignore-certificate-errors')
    driver = None  # Initialize driver to None
    try:
        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)
        driver.get(test_url)
        WebDriverWait(driver, 10).until(
            lambda d: d.execute_script("return document.readyState") == "complete"
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Page loaded.")
    except Exception as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error during driver creation or page load: {e}")
        if driver:
            driver.quit()
        raise # Re-raise the exception to stop the script if driver creation fails
    return driver

def run_test():
    driver = None  # Initialize driver to None for the finally block
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Config.toml is missing.")
        return

    test_url = config["url"] + "/tests/ui/cmpdecl_test.app"
    
    try:
        driver = create_driver(test_url)

        WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.TAG_NAME, "body"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Body element found.")

        WebDriverWait(driver, 20).until(
            lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded")
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Framework initialized (pg_isloaded).")

        try:
            # 1. text area. This selector targets a textarea inside a div whose ID starts with 'tx' AND ends with 'base'.
            # This is designed to handle dynamic IDs like "tx469base", "txXYZbase", etc.
            textarea_selector = "div[id^='tx'][id$='base'] textarea"
            wait_time_for_textarea = 20  # seconds to wait for the textarea

            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Attempting to find textarea with selector: {textarea_selector}")

            # Wait for the textarea to be present, visible, and clickable
            textarea = WebDriverWait(driver, wait_time_for_textarea).until(
                EC.element_to_be_clickable(
                    (By.CSS_SELECTOR, textarea_selector)
                )
            )
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Textarea found and clickable.")
            time.sleep(2)  # Pause to observe textarea presence
            textarea.click()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Textarea clicked.")
            time.sleep(1)  # Pause to observe click
            # Send initial keys
            textarea.send_keys("Hello! This is my fake password: 1234567890!@#$%^&*()_+-=[]{}|;':\",.<>?/")
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Initial text sent to textarea.")

            # Send random text
            random_text = ''.join(random.choices(
                'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012349!@#$%^&*()_+-=[]{}|;:\'",.<>?/~`', # Corrected '0123456789'
                k=1000 # Reduced k for potentially more stable rapid input; adjust as needed
            ))
            textarea.send_keys(random_text)
            time.sleep(5)  # Pause to observe the input
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Large random text sent to textarea.")
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Textarea input successful.")

        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Textarea not found or not clickable within {wait_time_for_textarea}s using selector: {textarea_selector}.")
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Check 'page_source_cmpdecl.html' to verify the textarea's presence, ID, and visibility.")
        except ElementClickInterceptedException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Textarea click was intercepted by another element. Selector: {textarea_selector}.")
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - An unexpected error occurred with the textarea: {e}")

        # 2. Get button text/titles and click the button multiple times
        try:
            button_div_selector = "div[id^='tb'][id$='pane']"
            wait_time_for_button = 20  # seconds
            number_of_clicks = 5 # Define how many times you want to click the button

            # --- Get Titles and Text Once Before Looping ---
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Locating button to get its text and container title.")
            
            # Wait for the button to be ready before we get its details
            button_div = WebDriverWait(driver, wait_time_for_button).until(
                EC.element_to_be_clickable((By.CSS_SELECTOR, button_div_selector))
            )

            # Get the button's own visible text (what it is "entitled")
            # The text is inside a <span> tag within the button's div
            try:
                button_text = button_div.find_element(By.TAG_NAME, 'span').text
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button is entitled: '{button_text}'")
            except Exception as e_text:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Could not get button's inner text: {e_text}")
                button_text = "[Not Found]"

            # --- Loop to Click the Button Multiple Times ---
            print(f"\n{datetime.now().strftime('%H:%M:%S.%f')} - Now starting to click the button {number_of_clicks} times.\n")
            time.sleep(1) # A brief pause before the clicks start

            for i in range(number_of_clicks):
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - --- Click Attempt #{i+1} ---")
                try:
                    # IMPORTANT: Re-find the element before each click in the loop.
                    # This prevents errors if the button gets refreshed after a click.
                    button_to_click = WebDriverWait(driver, wait_time_for_button).until(
                        EC.element_to_be_clickable((By.CSS_SELECTOR, button_div_selector))
                    )
                    
                    button_to_click.click()
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Click #{i+1} successful.")
                    
                    # Pause briefly to let the UI react to the click
                    time.sleep(0.5)

                except ElementClickInterceptedException:
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Click #{i+1} was intercepted. Trying a JS click fallback.")
                    # If a normal click is blocked, we can try clicking with JavaScript for this attempt.
                    # We use the already found 'button_to_click' element.
                    driver.execute_script("arguments[0].click();", button_to_click)
                    time.sleep(0.5) # Pause after JS click
                except TimeoutException:
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Button could not be found or became un-clickable before click #{i+1}. Stopping loop.")
                    break  # Exit the loop if the button disappears

            print(f"\n{datetime.now().strftime('%H:%M:%S.%f')} - Finished {number_of_clicks} click attempts.")

        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Could not find the button-like div initially. Aborting button test.")
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - An unexpected error occurred with the button: {e}")
    except Exception as e:
        # Catch any other exceptions that might occur before or during test steps (e.g., driver creation issues)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - A critical error occurred in run_test: {e}")
    
    finally:      
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test steps finished. Keeping browser open for observation (5s).")
            time.sleep(5) 
            driver.quit()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")
        
if __name__ == "__main__":
    run_test()