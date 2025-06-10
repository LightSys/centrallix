# David Hopkins May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.common.exceptions import TimeoutException
from datetime import datetime
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager

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
    """Run the image button functionality test with structured reporting."""
    print("# UI Test coverage: ImageButton Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/imagebutton_test.app"

        # --- Driver and Page Initialization ---
        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--ignore-certificate-errors')
        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Page Initialization ---
        init_test = TestBlock(1, "Page and Element Initialization")
        init_test.start()
        dynamic_base = None
        status_label_id = None
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page loaded successfully", True)
            
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework is initialized (pg_isloaded)", True)
            
            dynamic_base_xpath = "//*[starts-with(@id, 'al') and substring(@id, string-length(@id) - string-length('base') + 1) = 'base'][1]"
            dynamic_base = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.XPATH, dynamic_base_xpath)))
            init_test.add_check(f"dynamic container found (ID: {dynamic_base.get_attribute('id')})", True)
            
            status_label_xpath = "(.//div[starts-with(@id, 'lbl')])[1]"
            status_label_element = dynamic_base.find_element(By.XPATH, status_label_xpath)
            status_label_id = status_label_element.get_attribute('id')
            init_test.add_check(f"status label found (ID: {status_label_id})", True)

        except Exception as e:
            if not dynamic_base: init_test.add_check("finding dynamic container", False)
            if not status_label_id: init_test.add_check("finding status label", False)
            print(f"\n--- A CRITICAL ERROR OCCURRED during initialization ---\n{e}")
        all_blocks_passed.append(init_test.conclude())

        # This block contains all button interactions.
        interaction_test = TestBlock(2, "Button Interactions and Verifications")
        interaction_test.start()
        
        if dynamic_base and status_label_id:
            # --- TwoStateBtn Interaction ---
            try:
                two_state_btn_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[1]"
                two_state_btn = dynamic_base.find_element(By.XPATH, two_state_btn_xpath)
                ActionChains(driver).move_to_element(two_state_btn).perform()
                time.sleep(1)
                clickable_btn = WebDriverWait(driver, 10).until(EC.element_to_be_clickable(two_state_btn))
                clickable_btn.click()
                time.sleep(1)
                
                expected_text = "TwoStateBtn clicked!"
                WebDriverWait(driver, 10).until(EC.text_to_be_present_in_element((By.ID, status_label_id), expected_text))
                actual_text = driver.find_element(By.ID, status_label_id).text.strip()
                assert actual_text == expected_text
                interaction_test.add_check(f"TwoStateBtn click and verification", True)
            except Exception:
                interaction_test.add_check("TwoStateBtn click and verification", False)

            # --- ThreeStateBtn Interaction ---
            try:
                three_state_btn_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[2]"
                three_state_btn = dynamic_base.find_element(By.XPATH, three_state_btn_xpath)
                ActionChains(driver).move_to_element(three_state_btn).perform()
                time.sleep(1)
                clickable_btn = WebDriverWait(driver, 10).until(EC.element_to_be_clickable(three_state_btn))
                clickable_btn.click()
                time.sleep(1)
                
                expected_text = "ThreeStateBtn clicked!"
                WebDriverWait(driver, 10).until(EC.text_to_be_present_in_element((By.ID, status_label_id), expected_text))
                actual_text = driver.find_element(By.ID, status_label_id).text.strip()
                assert actual_text == expected_text
                interaction_test.add_check("ThreeStateBtn click and verification", True)
            except Exception:
                interaction_test.add_check("ThreeStateBtn click and verification", False)

            # --- EnableBtn Interaction ---
            try:
                enable_btn_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[4]"
                enable_btn = dynamic_base.find_element(By.XPATH, enable_btn_xpath)
                ActionChains(driver).move_to_element(enable_btn).perform()
                time.sleep(1)
                clickable_btn = WebDriverWait(driver, 10).until(EC.element_to_be_clickable(enable_btn))
                clickable_btn.click()
                time.sleep(1)
                
                expected_text = "Enabled DisabledBtn"
                WebDriverWait(driver, 10).until(EC.text_to_be_present_in_element((By.ID, status_label_id), expected_text))
                actual_text = driver.find_element(By.ID, status_label_id).text.strip()
                assert actual_text == expected_text
                interaction_test.add_check("EnableBtn click and verification", True)
            except Exception:
                interaction_test.add_check("EnableBtn click and verification", False)

            # --- Click the now-enabled DisabledBtn ---
            try:
                disabled_btn_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[3]"
                disabled_btn = dynamic_base.find_element(By.XPATH, disabled_btn_xpath)
                ActionChains(driver).move_to_element(disabled_btn).perform()
                time.sleep(1)
                clickable_btn = WebDriverWait(driver, 10).until(EC.element_to_be_clickable(disabled_btn))
                clickable_btn.click() # A single click is enough to prove it's enabled
                time.sleep(1)
                interaction_test.add_check("clicking the now-enabled 'DisabledBtn'", True)
            except Exception:
                interaction_test.add_check("clicking the now-enabled 'DisabledBtn'", False)

            # --- DisableBtn Interaction ---
            try:
                disable_btn_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[5]"
                disable_btn = dynamic_base.find_element(By.XPATH, disable_btn_xpath)
                ActionChains(driver).move_to_element(disable_btn).perform()
                time.sleep(1)
                clickable_btn = WebDriverWait(driver, 10).until(EC.element_to_be_clickable(disable_btn))
                clickable_btn.click()
                time.sleep(1)
                
                expected_text = "Disabled DisabledBtn"
                WebDriverWait(driver, 10).until(EC.text_to_be_present_in_element((By.ID, status_label_id), expected_text))
                actual_text = driver.find_element(By.ID, status_label_id).text.strip()
                assert actual_text == expected_text
                interaction_test.add_check("DisableBtn click and verification", True)
            except Exception:
                interaction_test.add_check("DisableBtn click and verification", False)
                
            # --- Final Verification of Disabled State ---
            try:
                disabled_btn_final_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[3]"
                disabled_btn_element_final_check = dynamic_base.find_element(By.XPATH, disabled_btn_final_xpath)
                time.sleep(1)
                
                is_effectively_disabled = False
                try:
                    WebDriverWait(driver, 3).until_not(EC.element_to_be_clickable(disabled_btn_element_final_check))
                    is_effectively_disabled = True
                except TimeoutException: # Element was still considered clickable
                    status_before_click = driver.find_element(By.ID, status_label_id).text.strip()
                    disabled_btn_element_final_check.click()
                    time.sleep(1)
                    status_after_click = driver.find_element(By.ID, status_label_id).text.strip()
                    if status_before_click == status_after_click:
                        is_effectively_disabled = True
                
                assert is_effectively_disabled
                interaction_test.add_check("final check confirms button is disabled", True)
            except Exception:
                interaction_test.add_check("final check confirms button is disabled", False)

        else:
            interaction_test.add_check("all button interactions skipped", False)
        all_blocks_passed.append(interaction_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 2:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"ImageButton Test {final_status}")
        print("---")
        if driver:
            print("Test complete. Browser will close in 5 seconds.")
            time.sleep(5)
            driver.quit()

if __name__ == "__main__":
    run_test()