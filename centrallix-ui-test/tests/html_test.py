# David Hopkins June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
from selenium.webdriver.common.keys import Keys

class TestBlock:
    """A class to manage a block of test checks and format the output."""
    def __init__(self, number, name):
        self.number = number
        self.name = name
        self.checks = []

    def start(self):
        """Prints the header for this test block."""
        print(f"TEST {self.number} = {self.name}")

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
        print(f"({passed_count}/{total_count}) {status}\n")
        return block_passed

def run_test():
    """Runs the entire suite of tests and provides a formatted summary."""
    print("# UI Test coverage: HTML Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/html_test.app"

        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')

        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Page Initialization and Content Verification ---
        content_test = TestBlock(1, "Page Initialization and Content Verification")
        content_test.start()
        
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            content_test.add_check("page loaded successfully", True)
        except Exception:
            content_test.add_check("page loaded successfully", False)

        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            content_test.add_check("framework is initialized (pg_isloaded)", True)
        except Exception:
            content_test.add_check("framework is initialized (pg_isloaded)", False)
        
        try:
            html_element = driver.find_element(By.TAG_NAME, "html")
            html_content = html_element.get_attribute('innerHTML')
            content_test.add_check(f"HTML content length is {len(html_content)} characters", True)
        except Exception:
            content_test.add_check("HTML content length could not be determined", False)

        try:
            title = driver.title
            h1_text = driver.find_element(By.TAG_NAME, "h1").text
            content_test.add_check(f"page title is '{title}' and H1 is '{h1_text}'", True)
        except Exception:
            content_test.add_check("page title and H1 could not be retrieved", False)

        all_blocks_passed.append(content_test.conclude())
        time.sleep(1)

        # --- TEST 2: Interactive Element Testing ---
        interaction_test = TestBlock(2, "Interactive Element Testing")
        time.sleep(2)
        interaction_test.start()
        text_field = None
        try:
            text_field = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.CSS_SELECTOR, "input[type='text']")))
            text_field.click()
            text_field.send_keys("LightSys Technology Services")
            interaction_test.add_check("sending text to input field", True)
            time.sleep(2)  # Pause to observe input
        except Exception:
            interaction_test.add_check("sending text to input field", False)

        if text_field:
            try:
                response_element = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.ID, "greetingMessage")))
                response_text = response_element.text
                interaction_test.add_check(f"response message received: '{response_text}'", True)
            except Exception:
                interaction_test.add_check("response message not found", False)

            try:
                current_value = text_field.get_attribute('value')
                text_field.send_keys(Keys.END)
                for _ in range(len(current_value)):
                    text_field.send_keys(Keys.BACK_SPACE)
                interaction_test.add_check("clearing text field using backspaces", True)
                time.sleep(2)
            except Exception:
                interaction_test.add_check("clearing text field using backspaces", False)

            try:
                response_element = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.ID, "greetingMessage")))
                response_text_after_clear = response_element.text
                is_empty = response_text_after_clear == ""
                interaction_test.add_check("response message is empty after clearing", is_empty)
                time.sleep(2)  # Pause to observe response
            except Exception:
                interaction_test.add_check("response message is empty after clearing", False)
        else:
            interaction_test.add_check("all further interactions (SKIPPED)", False)

        all_blocks_passed.append(interaction_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 2:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"HTML Test {final_status}")
        print("---")
        if driver:
            print("Test complete. Browser will close in 2 seconds.")
            time.sleep(2)
            driver.quit()

if __name__ == "__main__":
    run_test()