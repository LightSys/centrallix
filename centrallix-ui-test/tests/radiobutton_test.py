# David Hopkins June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.common.exceptions import TimeoutException, ElementClickInterceptedException
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
    """Runs the radio button test with structured reporting."""
    print("# UI Test coverage: RadioButton Click Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/radiobutton_test.app"

        # --- Driver and Page Initialization ---
        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')

        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Page Initialization and Panel Verification ---
        init_test = TestBlock(1, "Page Initialization and Panel Verification")
        init_test.start()
        
        radiobutton_panel = None
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page loaded successfully", True)
        except Exception:
            init_test.add_check("page loaded successfully", False)
        
        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework is initialized (pg_isloaded)", True)
        except Exception:
            init_test.add_check("framework is initialized (pg_isloaded)", False)

        try:
            radiobutton_panel = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.XPATH, "//div[starts-with(@id, 'rb') and contains(@id, 'parent')]")))
            title_div = radiobutton_panel.find_element(By.XPATH, ".//div[substring(@id, string-length(@id) - string-length('title') + 1) = 'title']//font")
            title_text = title_div.text
            init_test.add_check(f"radio button panel title is '{title_text}'", True)
        except Exception:
            init_test.add_check("radio button panel title could not be found", False)

        all_blocks_passed.append(init_test.conclude())

        # --- TEST 2: Clicking Radio Button Choices ---
        interaction_test = TestBlock(2, "Clicking Radio Button Choices")
        interaction_test.start()
        
        if radiobutton_panel:
            try:
                radio_options = radiobutton_panel.find_elements(By.XPATH, ".//div[starts-with(@id, 'rb') and contains(@id, 'option')]")
                interaction_test.add_check(f"found {len(radio_options)} radio button options", len(radio_options) > 0)
                
                # Loop to click each button and report the action
                for option in radio_options:
                    label = None
                    label_text = "[Unknown Label]"
                    try:
                        # Find the label to get its text and to click it
                        label = option.find_element(By.XPATH, ".//div[starts-with(@id, 'rb') and contains(@id, 'label')]")
                        label_text = label.text
                        
                        # Attempt to click the label
                        ActionChains(driver).move_to_element(label).click().perform()
                        time.sleep(0.5) # Brief pause for action to register
                        
                        # If the click action completes without error, it's a PASS
                        interaction_test.add_check(f"clicked choice: '{label_text}'", True)
                    
                    except ElementClickInterceptedException:
                        # Fallback logic if the click is blocked
                        try:
                            print(f"    (Info) Click on '{label_text}' was intercepted, trying JS fallback.")
                            driver.execute_script("arguments[0].click();", label)
                            time.sleep(0.5)
                            interaction_test.add_check(f"clicked choice: '{label_text}' (using JS)", True)
                        except Exception as js_e:
                            interaction_test.add_check(f"clicking choice: '{label_text}'", False)
                            print(f"    (Error) JS fallback click also failed: {js_e}")
                    
                    except Exception as e:
                        # Any other error during the click marks it as a failure
                        interaction_test.add_check(f"clicking choice: '{label_text}'", False)
                        print(f"    (Error) Could not click button: {e}")
            except Exception:
                 interaction_test.add_check("finding and looping through options", False)

        else:
            interaction_test.add_check("all interactions (SKIPPED)", False)
        
        all_blocks_passed.append(interaction_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 2:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"RadioButton Test {final_status}")
        print("---")
        sys.exit(0 if final_status == "PASS" else 1)

        if driver:
            print("Test complete. Browser will close in 2 seconds.")
            time.sleep(2)
            driver.quit()

if __name__ == "__main__":
    run_test()