# David Hopkins - June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
import random
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
import sys

class TestBlock:
    """A class to manage a block of test checks and format the output."""
    def __init__(self, number, name):
        self.number = number
        self.name = name
        self.checks = []

    def start(self):
        """Prints the header for this test block."""
        print(f"START TEST {self.number}")

    def add_check(self, description, passed: bool):
        """Adds a check to the block and prints its immediate status."""
        self.checks.append(passed)
        status = "PASS" if passed else "FAIL"
        print(f"    Test {description} ... {status}")

    def conclude(self) -> bool:
        """Prints the summary for the block and returns its overall status."""
        passed_count = sum(1 for p in self.checks if p)
        total_count = len(self.checks)
        block_passed = passed_count == total_count and total_count > 0
        status = "PASS" if block_passed else "FAIL"
        print(f"TEST {self.number} = {self.name} ({passed_count}/{total_count}) {status}\n")
        return block_passed

def run_test():
    """Runs the entire suite of tests and provides a formatted summary."""
    print("# UI Test coverage: Timer + Button Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/timerbutton_test.app"

        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')
        chrome_options.add_argument('--disable-dev-shm-usage')
        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Initialization ---
        init_test = TestBlock(1, "Page and Framework Initialization")
        init_test.start()
        
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == 'complete')
            init_test.add_check("page ready state is complete", True)
        except Exception as e:
            print(f"Error loading page: {e}")
            init_test.add_check("page ready state is complete", False)
        all_blocks_passed.append(init_test.conclude())

        # --- TEST 2: Coordinated Timer, Type, and Clear Action ---
        main_test = TestBlock(2, "Coordinated Timer, Type, and Clear Action")
        main_test.start()

        try:
            # Step 1: Find and click the button to start the 5-second timer.
            button_xpath = "//div[.//span[text()='Start Timer']]"
            button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, button_xpath)))
            button.click()
            main_test.add_check("Clicked 'Start Timer' to begin 5s countdown", True)
            
            # Step 2: Immediately find the text area using a robust locator (dynamic divs)
            textarea_xpath = "//textarea"
            text_area = WebDriverWait(driver, 5).until(EC.presence_of_element_located((By.XPATH, textarea_xpath)))
            main_test.add_check("Found text area post-click", True)

            # Step 3: Send random keys for 4 seconds.
            print("      Typing random characters for 4 seconds...")
            start_time = time.time()
            chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.?!@#$%^&*()_+-=[]{}|;':\",./<>?"
            total_sent = 0
            while time.time() - start_time < 4:
                random_chunk = "".join(random.choice(chars) for _ in range(100))
                text_area.send_keys(random_chunk)
                total_sent += len(random_chunk)
                time.sleep(0.01) # Small sleep to let UI keep up

            text_before_clear = text_area.get_attribute('value')
            main_test.add_check(f"Finished typing. Sent {total_sent} chars.", len(text_before_clear) == total_sent)
            
            # Step 4: Immediately clear the text area. This should be well within the 5s window.
            text_area.clear()
            text_after_clear = text_area.get_attribute('value')
            main_test.add_check("Text area cleared successfully", text_after_clear == "")

            # Step 5: Wait for the button timer to expire and verify its text has reset.
            print("      Waiting for button to reset...")
            reset_button_xpath = "//div[.//span[text()='Click Me Again']]"
            WebDriverWait(driver, 6).until(EC.presence_of_element_located((By.XPATH, reset_button_xpath)))
            main_test.add_check("Button text reset to 'Click Me Again' after 5s", True)
        
        except Exception as e:
            # This will catch any failure during the complex sequence.
            print(f"      An error occurred during the sequence: {e}")
            main_test.add_check("entire coordinated sequence", False)
        
        all_blocks_passed.append(main_test.conclude())
        
        # --- TEST 3 ---. Once after the click me button is reset, click it again, and redo the sequence again or 1 more time.
        main_test = TestBlock(3, "Redo Sequence After Button Reset (Click Me Again)")
        main_test.start()
        try:
            time.sleep(2)
            #Check 1 
            button_xpath = "//div[.//span[text()='Click Me Again']]"
            button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, button_xpath)))
            button.click()
            main_test.add_check("Clicked 'Click Me Again' to begin 5s countdown", True)

            #Check 2
            textarea_xpath = "//textarea"
            text_area = WebDriverWait(driver, 5).until(EC.presence_of_element_located((By.XPATH, textarea_xpath)))
            main_test.add_check("Found text area post-click", True)

            #Check 3: Make sure that the text area is empty 
            text_after_second_click = text_area.get_attribute('value')
            main_test.add_check("Text area is empty after second click", text_after_second_click == "")

            #Check 4: Send random keys for 4 seconds.
            print("      Typing random characters for 4 seconds...")
            start_time = time.time()
            chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.?!@#$%^&*()_+-=[]{}|;':\",./<>?"
            total_sent = 0 
            while time.time() - start_time < 4:
                random_chunk = "".join(random.choice(chars) for _ in range(100))
                text_area.send_keys(random_chunk)
                total_sent += len(random_chunk )
                time.sleep(0.01)
            text_before_clear = text_area.get_attribute('value')
            main_test.add_check(f"Finished typing. Sent {total_sent} chars.", len(text_before_clear) == total_sent)

            #Check 5: Immediately clear the text area. This should be well within the 5s window.
            text_area.clear()
            text_after_clear = text_area.get_attribute('value')
            main_test.add_check("Text area cleared successfully", text_after_clear == "")

            #Check 6: Check that the button text has reset, and that the the text area is empty. 
            print("      Waiting for button to reset...")
            reset_button_xpath = "//div[.//span[text()='Click Me Again']]"
            WebDriverWait(driver, 6).until(EC.presence_of_element_located((By.XPATH, reset_button_xpath)))
            button_text = driver.find_element(By.XPATH, reset_button_xpath).text
            main_test.add_check("Button text reset to 'Click Me Again' after 5s", button_text == "Click Me Again")
            

        except Exception as e:
            print(f"      An error occurred during the second sequence: {e}")
            main_test.add_check("entire second sequence", False)
        
        all_blocks_passed.append(main_test.conclude())


        # --- TEST 4 ---. Check for the HTML and make sure it is returning HTML elements.
        main_test = TestBlock(4, "Check HTML Elements After Button Reset")
        main_test.start()

        try:
            html_element = driver.find_element(By.TAG_NAME, "html")
            html_content = html_element.get_attribute('innerHTML')
            main_test.add_check(f"HTML content length is {len(html_content)} characters", True)
        except Exception:
            main_test.add_check("HTML content length could not be determined", False)

        try:
            title = driver.title
            # The message is in a div with id 'message'
            try:
                message_text = driver.find_element(By.ID, "message").text
                main_test.add_check(f"page title is '{title}', message is '{message_text}'", True)
            except Exception:
                main_test.add_check(f"page title is '{title}', but message could not be retrieved", False)
        except Exception:
            main_test.add_check("page title could not be retrieved", False)


        all_blocks_passed.append(main_test.conclude())

    finally:
        # Determine overall status by checking if all test blocks passed
        final_status = "PASS" if all(all_blocks_passed) and all_blocks_passed[0] and len(all_blocks_passed) > 1 and all_blocks_passed[1] else "FAIL"
        print(f"Timer + Button Widget Test {final_status}")
        print("---\n")
        
        if driver:
            print("Test complete. Browser will close in 5 seconds.")
            time.sleep(5)
            driver.quit()
        
        sys.exit(0 if final_status == "PASS" else 1)

if __name__ == "__main__":
    run_test()
