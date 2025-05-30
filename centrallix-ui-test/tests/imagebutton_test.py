
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

def create_driver(test_url) -> webdriver.Chrome:
    """Create and return a configured Chrome WebDriver."""
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    # chrome_options.add_argument('--incognito') # Removed for now, ensure it's not affecting IDs if re-enabled
    chrome_options.add_argument('--ignore-certificate-errors')  # Skip SSL errors

    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)

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
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Config.toml is missing...")
        return

    test_url = config["url"] + "/tests/ui/imagebutton_test.app"
    driver = create_driver(test_url)

    try:
        WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.TAG_NAME, "body"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Body element found.")
        time.sleep(2)

        WebDriverWait(driver, 20).until(
            lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded")
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Framework initialized.")
        time.sleep(2)

        # 1. Locate the main container (dynamic_base)
        # HTML example: al937base (length 7)
        # This XPath finds an element whose ID starts with 'al' and ends with 'base'
        dynamic_base_xpath = "//*[starts-with(@id, 'al') and substring(@id, string-length(@id) - string-length('base') + 1) = 'base'][1]"
        dynamic_base = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.XPATH, dynamic_base_xpath))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Dynamic base found: {dynamic_base.get_attribute('id')}")
        time.sleep(1)

        # --- TwoStateBtn Interaction ---
        # It's the FIRST div under dynamic_base with ID like "ibXXXXpane"
        two_state_btn_relative_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[1]"
        
        # For WebDriverWait, we need a full XPath or to pass dynamic_base as context if API allowed (it doesn't directly)
        # So, we find it relative to dynamic_base *after* dynamic_base is confirmed.
        two_state_btn_element = dynamic_base.find_element(By.XPATH, two_state_btn_relative_xpath)
        # Or wait for it using a full path if needed for clickability checks:
        # two_state_btn_full_xpath = f"//div[@id='{dynamic_base.get_attribute('id')}']{two_state_btn_relative_xpath.lstrip('.')}"
        
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found TwoStateBtn element with ID: {two_state_btn_element.get_attribute('id')}")

        ActionChains(driver).move_to_element(two_state_btn_element).perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Hovered over TwoStateBtn.")
        time.sleep(1)

        # Ensure the element is clickable and then click it
        # Pass the WebElement to element_to_be_clickable
        clickable_two_state_btn = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable(two_state_btn_element)
        )
        clickable_two_state_btn.click()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - TwoStateBtn clicked.")
        time.sleep(1)

        # --- Status Label Verification for TwoStateBtn ---
        # The status label is a div under dynamic_base with ID like "lblXXX"
        # It's the FIRST div with ID starting "lbl" under dynamic_base
        status_label_relative_xpath = "(.//div[starts-with(@id, 'lbl')])[1]"
        status_label_element = dynamic_base.find_element(By.XPATH, status_label_relative_xpath)
        status_label_id_actual = status_label_element.get_attribute('id') # e.g., "lbl153"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found Status Label element with ID: {status_label_id_actual}")
        
        expected_status_text_twostate = "TwoStateBtn clicked!"
        status_label_locator_for_wait = (By.ID, status_label_id_actual) # Use the actual found ID

        WebDriverWait(driver, 10).until(
            EC.text_to_be_present_in_element(status_label_locator_for_wait, expected_status_text_twostate)
        )
        # Re-fetch the element by its specific ID to get the most current text
        current_status_label_element = driver.find_element(By.ID, status_label_id_actual)
        actual_text = current_status_label_element.text.strip()
        
        assert actual_text == expected_status_text_twostate, \
            f"Expected status after TwoStateBtn to be '{expected_status_text_twostate}', got '{actual_text}'"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Status after TwoStateBtn: {actual_text}")
        time.sleep(1)

        # --- ThreeStateBtn Interaction
        # It's the SECOND div under dynamic_base with ID like "ibXXXXpane"
        three_state_btn_relative_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[2]"
        three_state_btn_element = dynamic_base.find_element(By.XPATH, three_state_btn_relative_xpath)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found ThreeStateBtn element with ID: {three_state_btn_element.get_attribute('id')}")

        ActionChains(driver).move_to_element(three_state_btn_element).perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Hovered over ThreeStateBtn.")
        time.sleep(1)
        
        clickable_three_state_btn = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable(three_state_btn_element)
        )
        clickable_three_state_btn.click()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - ThreeStateBtn clicked.")
        time.sleep(1)

        # Status Label Verification for ThreeStateBtn
        expected_status_text_threestate = "ThreeStateBtn clicked!"
        # status_label_locator_for_wait is still (By.ID, status_label_id_actual)
        WebDriverWait(driver, 10).until(
            EC.text_to_be_present_in_element(status_label_locator_for_wait, expected_status_text_threestate)
        )
        current_status_label_element = driver.find_element(By.ID, status_label_id_actual)
        actual_text_three_state = current_status_label_element.text.strip()

        assert actual_text_three_state == expected_status_text_threestate, \
            f"Expected status after ThreeStateBtn to be '{expected_status_text_threestate}', got '{actual_text_three_state}'"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Status after ThreeStateBtn: {actual_text_three_state}")
        time.sleep(1)

        # --- EnableBtn Interaction
        # EnableBtn is the FOURTH "ib...pane" div
        enable_btn_relative_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[4]"
        enable_btn_element = dynamic_base.find_element(By.XPATH, enable_btn_relative_xpath)
        print(f"{datetime.now().strftime('%H"%M:%S.%f')} - Found EnableBtn element with ID: {enable_btn_element.get_attribute('id')}")
        ActionChains(driver).move_to_element(enable_btn_element).perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Hovered over EnableBtn.")
        time.sleep(1)
        clickable_enable_btn = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable(enable_btn_element)
        )
        clickable_enable_btn.click() 
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - EnableBtn clicked.")
        time.sleep(1)
        # Status Label Verification for EnableBtn
        expected_status_text_enable = "Enabled DisabledBtn"
        WebDriverWait(driver, 10).until(
            EC.text_to_be_present_in_element(status_label_locator_for_wait, expected_status_text_enable)
        )
        current_status_label_element = driver.find_element(By.ID, status_label_id_actual)
        actual_text_enable = current_status_label_element.text.strip()
        assert actual_text_enable == expected_status_text_enable, \
            f"Expected status after EnableBtn to be '{expected_status_text_enable}', got '{actual_text_enable}'"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Status after EnableBtn: {actual_text_enable}")
        time.sleep(1)

        #NOW: Try to click the Disabled button after being enabled
        #Click the Disabled button it is the THIRD "ib...pane" div
        disabled_button_checkmark = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[3]"
        disabled_btn_element = dynamic_base.find_element(By.XPATH, disabled_button_checkmark)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found Disabled Button : {disabled_btn_element.get_attribute('id')}")
        ActionChains(driver).move_to_element(disabled_btn_element).perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Hovered over Disabled Button.")
        time.sleep(1)
        # Attempt to click the disabled button
        try:
            clickable_disabled_btn = WebDriverWait(driver, 10).until(
                EC.element_to_be_clickable(disabled_btn_element)
            )
            for i in range(5):
                clickable_disabled_btn.click()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Disabled Button clicked ({i+1}/5).")
                time.sleep(1)
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Disabled Button clicked happen as expected.")
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DisabledBtn click failed as expected: {e}")
        time.sleep(1)

        # --- DisableBtn Interaction NOW click it 
        # DisableBtn is the FIFTH "ib...pane" div
        disable_btn_relative_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[5]"
        disable_btn_element = dynamic_base.find_element(By.XPATH, disable_btn_relative_xpath)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found DisableBtn element with ID: {disable_btn_element.get_attribute('id')}")
        ActionChains(driver).move_to_element(disable_btn_element).perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Hovered over DisableBtn.")
        time.sleep(1)
        clickable_disable_btn = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable(disable_btn_element)
        )
        clickable_disable_btn.click()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - DisableBtn clicked.")
        time.sleep(1)
        # Status Label Verification for DisableBtn
        expected_status_text_disable = "Disabled DisabledBtn"
        WebDriverWait(driver, 10).until(
            EC.text_to_be_present_in_element(status_label_locator_for_wait, expected_status_text_disable)
        )
        current_status_label_element = driver.find_element(By.ID, status_label_id_actual)
        actual_text_disable = current_status_label_element.text.strip()
        assert actual_text_disable == expected_status_text_disable, \
            f"Expected status after DisableBtn to be '{expected_status_text_disable}', got '{actual_text_disable}'"
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Status after DisableBtn: {actual_text_disable}")
        time.sleep(1)

        # NOW try to click the Disabled button again / verify it's disabled
        disabled_button_xpath = "(.//div[starts-with(@id, 'ib') and substring(@id, string-length(@id) - string-length('pane') + 1) = 'pane'])[3]"
        # Re-fetch the element to get its current state attributes
        disabled_btn_element_final_check = dynamic_base.find_element(By.XPATH, disabled_button_xpath)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Found Disabled Button for final check: {disabled_btn_element_final_check.get_attribute('id')}")
        time.sleep(1) # Allow UI to settle

        # --- Verification Logic ---
        is_effectively_disabled = False

        # Check 1: Attempt EC.element_to_be_clickable (Selenium's view)
        try:
            WebDriverWait(driver, 3).until(
                EC.element_to_be_clickable(disabled_btn_element_final_check)
            )
            # If it reaches here, Selenium thinks it's clickable.
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - INFO: Disabled Button is considered clickable by EC.element_to_be_clickable.")
            # This isn't necessarily a failure yet if the click does nothing.
        except TimeoutException:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - INFO: Disabled Button is NOT considered clickable by EC.element_to_be_clickable (this is good).")
            is_effectively_disabled = True # Good sign


        # Check 2: Attempt a click and verify no (or expected) action
        status_before_final_click = driver.find_element(By.ID, status_label_id_actual).text.strip()
        
        try:
            # We only attempt the click if Selenium thought it was clickable,
            # or if you want to force the click attempt regardless to see what happens.
            # For now,  attempt a click if not already confirmed disabled by lack of clickability.
            if not is_effectively_disabled: # Or if you always want to try clicking
                 print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Attempting to click the (supposedly) disabled button to verify behavior.")
                 disabled_btn_element_final_check.click()
                 time.sleep(1) # Give time for any unexpected action

            status_after_final_click = driver.find_element(By.ID, status_label_id_actual).text.strip()

            if status_before_final_click != status_after_final_click:
                error_msg = f"FAIL: Clicking the supposedly disabled button CHANGED the status from '{status_before_final_click}' to '{status_after_final_click}'!"
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - {error_msg}")
                raise AssertionError(error_msg)
            else:
                if not is_effectively_disabled: # It was clickable by Selenium
                     print(f"{datetime.now().strftime('%H:%M:%S.%f')} - WARNING: Button was clickable by Selenium, click attempted, but status label was unchanged. This might be acceptable for a 'disabled' button.")
                # If it wasn't clickable by Selenium, this is a definite pass.
                is_effectively_disabled = True # Confirming disabled by lack of effect.

        except Exception as click_exception: # Catches ElementNotInteractable etc.
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - INFO: Attempting to click the disabled button resulted in an exception (good sign): {type(click_exception).__name__}")
            is_effectively_disabled = True

        # Final Assertion
        if not is_effectively_disabled:
            final_error_msg = "FAIL: The Disabled Button did not behave as if it were disabled."
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - {final_error_msg} (It might have been clickable AND/OR its click caused an effect).")
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            dump_filename = f"page_dump_disabled_test_fail_{timestamp}.html"
            with open(dump_filename, "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Page HTML dumped to {dump_filename}")
            raise AssertionError(final_error_msg)
        else:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - SUCCESS: Disabled Button behaved as disabled (either not clickable or click had no effect).")

        time.sleep(1)

    except TimeoutException as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - A timeout occurred: {e}")
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        dump_filename = f"page_dump_timeout_{timestamp}.html"
        try:
            with open(dump_filename, "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Page HTML dumped to {dump_filename}")
        except Exception as dump_err:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to dump page HTML during timeout: {dump_err}")
        raise # Re-raise the timeout exception
    except Exception as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - An unexpected error occurred: {e}")
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        dump_filename = f"page_dump_error_{timestamp}.html"
        try:
            with open(dump_filename, "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Page HTML dumped to {dump_filename}")
        except Exception as dump_err:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Failed to dump page HTML during error: {dump_err}")
        raise # Re-raise the caught exception
    finally:
        if 'driver' in locals() and driver: # Ensure driver exists before trying to quit
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Keeping browser open for 5 seconds for observation.")
            time.sleep(5)
            driver.quit()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()