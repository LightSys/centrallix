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

    test_url = config["url"] + "/tests/ui/radiobutton_test.app"
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

        # 1. Get the page header (title tag)
        page_title = driver.title
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Page header (title): {page_title}")

        # 2. Get the radiobutton panel title
        try:
            # Locate the radiobutton panel parent div (dynamic ID starting with 'rb')
            radiobutton_panel = WebDriverWait(driver, 10).until(
                EC.presence_of_element_located((By.XPATH, "//div[starts-with(@id, 'rb') and contains(@id, 'parent')]"))
            )
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Radiobutton panel parent found: {radiobutton_panel.get_attribute('id')}")

            # Locate the title div within the panel (dynamic ID ending with 'title')
            title_div = radiobutton_panel.find_element(By.XPATH, ".//div[substring(@id, string-length(@id) - string-length('title') + 1) = 'title']//font")
            title_text = title_div.text
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Radiobutton title found: {title_text}")

        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to locate radiobutton title. Element not found.")
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error locating radiobutton title: {str(e)}")

        # 3. Click all radiobuttons in the panel
        try:
            # Locate all radiobutton option divs (e.g., rb34option1, rb34option2, etc.)
            radiobutton_options = radiobutton_panel.find_elements(By.XPATH, ".//div[starts-with(@id, 'rb') and contains(@id, 'option')]")
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found {len(radiobutton_options)} radiobuttons.")

            for index, option in enumerate(radiobutton_options, 1):
                try:
                    # Find the label div within the option (e.g., rb34label1)
                    label = option.find_element(By.XPATH, ".//div[starts-with(@id, 'rb') and contains(@id, 'label')]")
                    label_text = label.text
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Clicking radiobutton {index}: {label_text}")

                    # Click the label to select the radiobutton
                    ActionChains(driver).move_to_element(label).click().perform()
                    time.sleep(1)  # Pause to allow JavaScript to process the click

                    # Verify the radiobutton is selected by checking the buttonset div's visibility
                    buttonset = option.find_element(By.XPATH, ".//div[starts-with(@id, 'rb') and contains(@id, 'buttonset')]")
                    is_selected = buttonset.value_of_css_property("visibility") == "inherit"
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Radiobutton {index} selected: {is_selected}")

                except ElementClickInterceptedException:
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Radiobutton {index} click intercepted, retrying with JavaScript.")
                    driver.execute_script("arguments[0].click();", label)
                    time.sleep(1)
                    buttonset = option.find_element(By.XPATH, ".//div[starts-with(@id, 'rb') and contains(@id, 'buttonset')]")
                    is_selected = buttonset.value_of_css_property("visibility") == "inherit"
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Radiobutton {index} selected (via JS): {is_selected}")

                except Exception as e:
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error clicking radiobutton {index}: {str(e)}")

        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to locate radiobuttons. Element not found.")
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error locating radiobuttons: {str(e)}")

    except Exception as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test failed: {str(e)}")

    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete, keeping browser open for observation.")
        time.sleep(2)
        driver.quit()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()