# David Hopkins June 2025
# CMPDECL Part 2 Test
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
    print("# UI Test coverage: CMPDECL Part 2 Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []
    exit_code = 1 # Default to fail

    try:
        # Load config and set up WebDriver
        try:
            config = toml.load("config.toml")
            test_url = config["url"] + "/tests/ui/cmpdecl/maindeclcmp_test.app"
        except FileNotFoundError:
            print("ERROR: config.toml not found. Please create it.")
            sys.exit(1)

        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')
        
        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Page and Panel Initialization ---
        init_test = TestBlock(1, "Page and Panel Initialization")
        init_test.start()
        
        radiobutton_panel = None
        try:
            driver.get(test_url)
            WebDriverWait(driver, 15).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page loaded successfully", True)
        except Exception as e:
            init_test.add_check(f"page loading (Error: {e})", False)

        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework is initialized (pg_isloaded)", True)
        except Exception as e:
            init_test.add_check(f"framework initialization (Error: {e})", False)

        try:
            radiobutton_panel = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.XPATH, "//div[starts-with(@id, 'rb') and contains(@id, 'parent')]")))
            init_test.add_check("radio button panel container is present", True)
        except Exception:
            init_test.add_check("radio button panel container is present", False)

        all_blocks_passed.append(init_test.conclude())

        # --- TEST 2: Iterate and Click Choices ---
        interaction_test = TestBlock(2, "Iterate and Click Choices")
        interaction_test.start()
        
        if radiobutton_panel:
            try:
                radio_options = radiobutton_panel.find_elements(By.XPATH, ".//div[starts-with(@id, 'rb') and contains(@id, 'option')]")
                interaction_test.add_check(f"found {len(radio_options)} radio button options", len(radio_options) > 0)
                
                for option in radio_options:
                    label_text = "[Unknown Label]"
                    try:
                        label = option.find_element(By.XPATH, ".//div[contains(@id, 'label')]")
                        label_text = label.text.strip()
                        ActionChains(driver).move_to_element(label).click().perform()
                        time.sleep(0.5)
                        interaction_test.add_check(f"clicked choice: '{label_text}'", True)
                    
                    except ElementClickInterceptedException:
                        print(f"    (Info) Click on '{label_text}' was intercepted, trying JS fallback.")
                        driver.execute_script("arguments[0].click();", label)
                        time.sleep(0.5)
                        interaction_test.add_check(f"clicked choice: '{label_text}' (using JS)", True)

                    except Exception as e:
                        interaction_test.add_check(f"clicking choice: '{label_text}'", False)
                        print(f"    (Error) Could not click '{label_text}'. Reason: {str(e).splitlines()[0]}")
                        break 
            
            except Exception as e:
                interaction_test.add_check("locating and looping through options", False)
                print(f"    (Error) A problem occurred during the interaction loop setup: {e}")
        else:
            interaction_test.add_check("all interactions (SKIPPED)", False)
        
        all_blocks_passed.append(interaction_test.conclude())

        # --- TEST 3: Table Data Extraction and Formatting ---
        table_test = TestBlock(3, "Table Data Extraction and Formatting")
        table_test.start()
        try:
            table_pane_locator = (By.XPATH, "//div[contains(@id, 'tbld') and contains(@id, 'pane')]")
            table_pane = WebDriverWait(driver, 10).until(EC.presence_of_element_located(table_pane_locator))
            table_test.add_check("table container is present", True)

            # Extract headers from the specific gray header row
            header_row_locator = (By.XPATH, ".//div[contains(@style, 'c0c0c0') or contains(@style, '192, 192, 192')]")
            header_row = WebDriverWait(table_pane, 5).until(EC.presence_of_element_located(header_row_locator))
            header_elements = header_row.find_elements(By.XPATH, ".//span")
            headers = [h.text for h in header_elements if h.text]
            table_test.add_check(f"found {len(headers)} headers", len(headers) > 0)
            
            # Extract data rows. These are distinct divs, often siblings to each other.
            data_row_locator = (By.XPATH, ".//div[contains(@style, 'overflow: hidden')]//div[contains(@style, 'z-index: 1')]")
            row_elements = WebDriverWait(table_pane, 5).until(EC.presence_of_all_elements_located(data_row_locator))
            table_test.add_check(f"found {len(row_elements)} data rows", len(row_elements) > 0)
            
            # Process and print the data in the desired format
            print("    --- Formatted Table Data ---")
            for row in row_elements:
                cell_elements = row.find_elements(By.XPATH, ".//span")
                row_data = [cell.text for cell in cell_elements]
                
                # Ensure row_data has enough elements to prevent index errors
                if len(row_data) >= len(headers):
                    print(f"    {headers[0]} {row_data[0]} = {headers[1]} {row_data[1]}")
                else:
                    # Handle rows that might be missing data, like the last empty one
                    print(f"    - Found partial row: {row_data}")
            print("    --------------------------")


        except Exception as e:
            table_test.add_check("table data extraction", False)
            print(f"    (Error) Could not process table. Reason: {str(e).splitlines()[0]}")

        all_blocks_passed.append(table_test.conclude())

        if all(all_blocks_passed):
            exit_code = 0

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        exit_code = 1

    finally:
        if driver:
            print("Test complete. Browser will close in 5 seconds.")
            time.sleep(5)
            driver.quit()
        
        final_status = "PASS" if exit_code == 0 else "FAIL"
        print("---")
        print(f"CMPDECL Part 2 Test FINAL STATUS: {final_status}")
        print("---")
        sys.exit(exit_code)

if __name__ == "__main__":
    run_test()