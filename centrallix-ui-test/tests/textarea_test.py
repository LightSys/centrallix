
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

    test_url = config["url"] + "/tests/ui/textarea_test.app"
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

        # Locate the textarea element
        textarea = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.CSS_SELECTOR, "textarea"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Textarea found.")
        time.sleep(2)  # Pause to observe textarea
        textarea.click()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Textarea clicked.")
        time.sleep(1)  # Pause to observe click
        long_text = (
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
            "1234567890!@#$%^&*()_+-=[]{}|;:',.<>/?~` "
            "Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; "
            "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
            "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. "
            "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
            "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. "
            "The quick brown fox jumps over the lazy dog. "
            "9876543210)(*&^%$#@!~`?><:{}[]|\\;'/.,mnbvcxzlkjhgfdsapoiuytrewq "
            "Testing special characters: ©®™✓∆π÷×¶∆§∞¢£¥•ªº–≠≤≥± "
            "End of test string. "
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ "
            "Another line with numbers: 11223344556677889900 "
            "Symbols: !@#$%^&*()_+-=<>?:\"{}|,./;'[]\\`~ "
            "This is a test input for the textarea. "
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
            "Phasellus imperdiet, nulla et dictum interdum, nisi lorem egestas odio, vitae scelerisque enim ligula venenatis dolor. "
            "Maecenas nisl est, ultrices nec congue eget, auctor vitae massa. "
            "Fusce luctus vestibulum augue ut aliquet. "
            "Nunc sagittis dictum nisi, sed ullamcorper ipsum dignissim ac. "
            "In at libero sed nunc venenatis imperdiet sed ornare turpis. "
            "Donec vitae dui eget tellus gravida venenatis. "
            "Integer fringilla congue eros non fermentum. "
            "Sed dapibus pulvinar nibh tempor porta. "
            "Cras ac leo purus. Mauris quis diam velit."
            " Here are some additional words to further extend the test input for the textarea. "
            " This should help ensure that the textarea can handle even longer strings without issues. "
            " Adding more sentences, numbers like 1010101010, and symbols such as %^&*() to increase complexity. "
            " The quick brown fox jumps over the lazy dog again, just for good measure. "
            " End of extended test input. "
            "Now adding even more words to make the input string longer and more robust for testing. "
            "This sentence is included to further increase the length of the test string. "
            "We can add more random words: apple banana cherry date elderberry fig grape honeydew. "
            "Let's include some technical terms: algorithm, binary, cache, database, encryption, framework, gateway, hash, index, JSON, kernel, latency, middleware, node, object, protocol, query, recursion, stack, thread, Unicode, variable, web, XML, yield, zero-day. "
            "Adding a few more sentences to ensure the textarea is thoroughly tested with a large input. "
            "Testing edge cases, boundary conditions, and overflow scenarios. "
            "Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. "
            "Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. "
            "Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. "
            "The end of the really long test string for the textarea input."
        )

        textarea.send_keys(long_text)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Long text entered into textarea.")
        time.sleep(2)  # Pause to observe text entry

        # Clear the textarea
        textarea.clear()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Textarea cleared.")
        time.sleep(2)  # Pause to observe clearing

        # Attempt to click the second textarea (if present)
        try:
            textareas = driver.find_elements(By.CSS_SELECTOR, "textarea")
            if len(textareas) < 2:
                 print(f"{datetime.now().strftime('%H:%M:%S.%f')} - There is no second textarea to click.")
            else:
                 second_textarea = textareas[1]
            try:
                second_textarea.click()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Successfully clicked the second textarea.")
                # After clicking the second textarea, print its current value (if any)
                value = second_textarea.get_attribute("value")
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Second textarea value after click: '{value}'")
                time.sleep(2)  # Pause to observe
            except Exception as e:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - The second textarea cannot be clicked. Exception: {e}")
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error locating textareas. Exception: {e}")
        

    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete, keeping browser open for observation.")
        time.sleep(2)
        driver.quit()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()
