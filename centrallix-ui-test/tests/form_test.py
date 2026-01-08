# David Hopkins June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.common.exceptions import TimeoutException
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
import sys
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.keys import Keys


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
        print(f"      Test {description} ... {status}")

    def conclude(self) -> bool:
        """Prints the summary for the block and returns its overall status."""
        passed_count = sum(1 for p in self.checks if p)
        total_count = len(self.checks)
        block_passed = passed_count == total_count and total_count > 0
        status = "PASS" if block_passed else "FAIL"
        print(f"TEST {self.number} = {self.name} ({passed_count}/{total_count}) {status}\n")
        return block_passed

def verify_and_print_data(driver: webdriver.Chrome, test_block: TestBlock, expected_first: str = None, expected_last: str = None):
    """
    A reusable function to verify and print user data from the form.
    """
    try:
        wait = WebDriverWait(driver, 10)
        
        print("\n--- Verifying Person Details ---")

        def get_field_value(field_label: str) -> str:
            """Finds a field, waits for its value, and returns it."""
            try:
                input_xpath = f"//div[p/span[text()='{field_label}']]/following-sibling::div[1]//input"
                input_element = wait.until(EC.visibility_of_element_located((By.XPATH, input_xpath)))
                wait.until(lambda d: input_element.get_attribute('value') != "", f"Value for '{field_label}' did not appear in time.")
                return input_element.get_attribute('value')
            except TimeoutException:
                return ""

        first_name_value = get_field_value("First Name:")
        print(f"  First Name: {first_name_value}")
        test_block.add_check(f"First Name contains '{expected_first}'", expected_first in first_name_value if expected_first else bool(first_name_value))

        last_name_value = get_field_value("Last Name:")
        print(f"  Last Name: {last_name_value}")
        test_block.add_check(f"Last Name contains '{expected_last}'", expected_last in last_name_value if expected_last else bool(last_name_value))

        email_value = get_field_value("Email:")
        print(f"  Email: {email_value}")
        test_block.add_check("Email value extracted", bool(email_value))

        print("\n--- Verifying Associated Computers ---")
        computers_pane_xpath = "//span[text()='Associated Computers']/ancestor::div[contains(@id, 'pn')]"
        header_xpath = f"{computers_pane_xpath}//div[contains(@id, 'tbld')]//div[contains(@style, 'height: 22px')]//span"
        rows_xpath = f"{computers_pane_xpath}//div[contains(@id, 'tbld')]//div[contains(@style, 'overflow: hidden')]/div/div"
        
        try:
            wait.until(EC.visibility_of_element_located((By.XPATH, rows_xpath)))
            header_elements = driver.find_elements(By.XPATH, header_xpath)
            header_map = {el.text.replace('\xa0', ' '): i for i, el in enumerate(header_elements)}
            rows = driver.find_elements(By.XPATH, rows_xpath)
            
            if not rows:
                print("  No associated computers found.")
                test_block.add_check("Verified that no computer data was present", True)
            else:
                print(f"  Found {len(rows)} computer(s):")
                for index, row in enumerate(rows):
                    cells = row.find_elements(By.XPATH, ".//span")
                    owner = cells[header_map.get('First Name')].text if 'First Name' in header_map and len(cells) > header_map.get('First Name') else 'N/A'
                    name = cells[header_map.get('Computer Name')].text if 'Computer Name' in header_map and len(cells) > header_map.get('Computer Name') else 'N/A'
                    memory = cells[header_map.get('MB Memory')].text if 'MB Memory' in header_map and len(cells) > header_map.get('MB Memory') else 'N/A'
                    print(f"    - Owner: {owner}, Name: {name}, Memory: {memory} MB")
                test_block.add_check(f"Successfully extracted data for {len(rows)} computer(s)", True)
        except TimeoutException:
            print("  No associated computers found.")
            test_block.add_check("Verified that no computer data was present", True)
    except Exception as e:
        test_block.add_check(f"Verification test encountered a critical error: {e}", False)

def handle_alert_if_present(driver: webdriver.Chrome, timeout: int = 3):
    """Waits for a JavaScript alert, accepts it, and prints its text."""
    try:
        WebDriverWait(driver, timeout).until(EC.alert_is_present())
        alert = driver.switch_to.alert
        alert_text = alert.text
        print(f"  Alert Found: '{alert_text}'. Accepting it.")
        alert.accept()
    except TimeoutException:
        print("  No alert was found, continuing.")
        pass

def run_test():
    """Runs the pane test with structured reporting."""
    print("# UI Test coverage: Form + Table Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/form/form_test.app"
        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')
        chrome_options.add_argument('--start-maximized')
        chrome_options.add_experimental_option('excludeSwitches', ['enable-logging'])
        driver = webdriver.Chrome(service=service, options=chrome_options)

        # TEST 1: Page Initialization and Form Interaction
        pane_test = TestBlock(1, "Page Initialization")
        pane_test.start()
        driver.get(test_url)
        WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
        pane_test.add_check("page loaded successfully", True)
        all_blocks_passed.append(pane_test.conclude())

        # TEST 2: Search and Verify User Data (First Name)
        search_frank_test = TestBlock(2, "Search for 'Frank'")
        search_frank_test.start()
        search_button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Search']")))
        search_button.click()
        first_name_input_xpath = "//div[p/span[text()='First Name:']]/following-sibling::div[1]//input"
        first_name_input = WebDriverWait(driver, 10).until(EC.visibility_of_element_located((By.XPATH, first_name_input_xpath)))
        first_name_input.clear()
        first_name_input.send_keys("Frank")
        search_button.click()
        search_frank_test.add_check("Search for 'Frank' submitted", True)
        time.sleep(2)
        all_blocks_passed.append(search_frank_test.conclude())

        # TEST 3: Verify 'Frank' Data
        verify_frank_test = TestBlock(3, "Verify 'Frank' Data")
        verify_frank_test.start()
        verify_and_print_data(driver, verify_frank_test, expected_first="Frank")
        time.sleep(2)
        all_blocks_passed.append(verify_frank_test.conclude())

        # TEST 4: Search for 'Allister' (Last Name  )
        search_allister_test = TestBlock(4, "Search for 'Allister'")
        search_allister_test.start()
        search_button.click()
        last_name_input_xpath = "//div[p/span[text()='Last Name:']]/following-sibling::div[1]//input"
        last_name_input = WebDriverWait(driver, 10).until(EC.visibility_of_element_located((By.XPATH, last_name_input_xpath)))
        last_name_input.clear()
        last_name_input.send_keys("Allister")
        search_button.click()
        search_allister_test.add_check("Search for 'Allister' submitted", True)
        time.sleep(2)
        all_blocks_passed.append(search_allister_test.conclude())

        # TEST 5: Verify 'Allister' Data
        verify_allister_test = TestBlock(5, "Verify 'Allister' Data")
        verify_allister_test.start()
        verify_and_print_data(driver, verify_allister_test, expected_last="Allister")
        time.sleep(2)
        all_blocks_passed.append(verify_allister_test.conclude())

        # TEST 6: Search for 'Erin' (Email)
        search_erin_test = TestBlock(6, "Search for 'Erin'")
        search_erin_test.start()
        search_button.click()
        email_input_xpath = "//div[p/span[text()='Email:']]/following-sibling::div[1]//input"
        email_input = WebDriverWait(driver, 10).until(EC.visibility_of_element_located((By.XPATH, email_input_xpath)))
        email_input.clear()
        email_input.send_keys("erin@nodomain.nowhere")
        search_button.click()
        search_erin_test.add_check("Search for 'Erin' submitted", True)
        time.sleep(2)
        all_blocks_passed.append(search_erin_test.conclude())

        # TEST 7: Verify 'Erin' Data
        verify_erin_test = TestBlock(7, "Verify 'Erin' Data")
        verify_erin_test.start()
        verify_and_print_data(driver, verify_erin_test, expected_first="Erin", expected_last="Erinste")
        time.sleep(2)
        all_blocks_passed.append(verify_erin_test.conclude())
        
        # BEGIN CRUD DESIGN TESTS
        # TEST 8: Create New User (With 4 Steps)
        new_user_test = TestBlock(8, "Create New User")
        new_user_test.start()
        try:
            wait = WebDriverWait(driver, 10)
            print("  Step 0: Resetting form to a clean state.")
            search_button = wait.until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Search']")))
            search_button.click()
            search_button.click()
            wait.until(EC.element_to_be_clickable((By.XPATH, "//span[text()='New']")))
            new_user_test.add_check("Form reset to a clean state", True)
            
            print("  Step 1: Clicking 'New' to prepare the form.")
            new_button = wait.until(EC.element_to_be_clickable((By.XPATH, "//span[text()='New']")))
            new_button.click()
            wait.until(EC.text_to_be_present_in_element_value((By.XPATH, first_name_input_xpath), ""))
            new_user_test.add_check("Form is ready for new user input", True)

            print("  Step 2: Entering new user details.")
            driver.find_element(By.XPATH, first_name_input_xpath).send_keys("Colorado")
            driver.find_element(By.XPATH, last_name_input_xpath).send_keys("Smith")
            driver.find_element(By.XPATH, email_input_xpath).send_keys("colorado.smith@hello.com")
            new_user_test.add_check("New user data entered into fields", True)

            print("  Step 3: Saving the new user.")
            save_button = wait.until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Save']")))
            save_button.click()
            handle_alert_if_present(driver)
            
            print("  Step 4: Verifying the new user appears in the main 'People' list.")
            new_user_in_table_xpath = "//span[text()='People']/ancestor::div[contains(@id, 'pn')]//span[text()='Colorado']"
            wait.until(EC.visibility_of_element_located((By.XPATH, new_user_in_table_xpath)))
            new_user_test.add_check("New user 'Colorado' is visible in the main table", True)
        except Exception as e:
            new_user_test.add_check(f"Create New User test failed: {e}", False)
        all_blocks_passed.append(new_user_test.conclude())

        # TEST 9: Verify 'Colorado Smith' Data
        verify_colorado_test = TestBlock(9, "Verify 'Colorado Smith' Data")
        verify_colorado_test.start()
        try:
            print("  Searching for the newly created user 'Colorado'.")
            search_button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Search']")))
            search_button.click()
            first_name_input = WebDriverWait(driver, 10).until(EC.visibility_of_element_located((By.XPATH, first_name_input_xpath)))
            first_name_input.clear()
            first_name_input.send_keys("Colorado")
            search_button.click()
            verify_colorado_test.add_check("Search for 'Colorado' submitted", True)
            verify_and_print_data(driver, verify_colorado_test, expected_first="Colorado", expected_last="Smith")
        except Exception as e:
            verify_colorado_test.add_check(f"Verify new user test failed: {e}", False)
        all_blocks_passed.append(verify_colorado_test.conclude())

        # TEST 10: Edit Existing User 
        edit_user_test = TestBlock(10, "Edit Existing User")
        edit_user_test.start()
        try:
            wait = WebDriverWait(driver, 10)
            print("  Step 1: Clicking 'Edit' to enable form fields.")
            edit_button = wait.until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Edit']")))
            edit_button.click()
            save_button_xpath = "//span[text()='Save']"
            wait.until(EC.element_to_be_clickable((By.XPATH, save_button_xpath)))
            edit_user_test.add_check("Form is in edit mode", True)
            
            print("  Step 2: Changing names.")
            last_name_input_element = driver.find_element(By.XPATH, last_name_input_xpath)
            last_name_input_element.clear()
            last_name_input_element.send_keys("Smithster")
            first_name_input_element = driver.find_element(By.XPATH, first_name_input_xpath)
            first_name_input_element.clear()
            first_name_input_element.send_keys("Coca-Cola")
            edit_user_test.add_check("First and Last names changed", True) 

            print("  Step 3: Saving the changes.")
            driver.find_element(By.XPATH, save_button_xpath).click()
            handle_alert_if_present(driver)

            print("  Step 4: Verifying the updated user appears in the main 'People' list.")
            updated_user_in_table_xpath = "//span[text()='People']/ancestor::div[contains(@id, 'pn')]//span[text()='Smithster']"
            wait.until(EC.visibility_of_element_located((By.XPATH, updated_user_in_table_xpath)))
            edit_user_test.add_check("Updated last name 'Smithster' is visible in the main table", True)
        except Exception as e:
            edit_user_test.add_check(f"Edit User test failed: {e}", False)
        all_blocks_passed.append(edit_user_test.conclude())

        # TEST 11: Verify Edited User Data
        verify_edit_test = TestBlock(11, "Verify 'Coca-Cola Smithster' Data")
        verify_edit_test.start()
        try:
            wait = WebDriverWait(driver, 10)
            print("  Searching for the user again to ensure data persistence.")
            search_button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Search']")))
            search_button.click()
            first_name_input = WebDriverWait(driver, 10).until(EC.visibility_of_element_located((By.XPATH, first_name_input_xpath)))
            first_name_input.clear()
            first_name_input.send_keys("Coca-Cola")
            search_button.click()
            verify_edit_test.add_check("Search for 'Coca-Cola' submitted", True)
            
            print("  Waiting for search results to populate the form...")
            wait.until(EC.text_to_be_present_in_element_value((By.XPATH, first_name_input_xpath), "Coca-Cola"))
            
            verify_and_print_data(driver, verify_edit_test, expected_first="Coca-Cola", expected_last="Smithster")
        except Exception as e:
            verify_edit_test.add_check(f"Verify edited user test failed: {e}", False)
        all_blocks_passed.append(verify_edit_test.conclude())

        # TEST 12: Delete the User ---
        if all_blocks_passed[-1]: # Only run delete if the last verify step passed
            delete_user_test = TestBlock(12, "Delete User")
            delete_user_test.start()
            try:
                wait = WebDriverWait(driver, 10)
                print("  Step 1: Clicking 'Delete' button.")
                delete_button = wait.until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Delete']")))
                delete_button.click()
                handle_alert_if_present(driver, timeout=5)
                delete_user_test.add_check("Delete initiated and confirmation alert handled", True)

                print("  Step 2: Verifying the user is removed from the main 'People' list.")
                deleted_user_in_table_xpath = "//span[text()='People']/ancestor::div[contains(@id, 'pn')]//span[text()='Smithster']"
                wait.until(EC.invisibility_of_element_located((By.XPATH, deleted_user_in_table_xpath)))
                delete_user_test.add_check("User 'Coca-Cola Smithster' is no longer visible in the table", True)

            except Exception as e:
                delete_user_test.add_check(f"Delete User test failed: {e}", False)
            all_blocks_passed.append(delete_user_test.conclude())

        # TEST 13: Verify User is Gone 
        verify_deletion_test = TestBlock(13, "Verify User is Deleted")
        verify_deletion_test.start()
        try:
            wait = WebDriverWait(driver, 10)
            print("  Searching for 'Coca-Cola' one last time to confirm deletion.")
            search_button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Search']")))
            search_button.click()
            first_name_input = WebDriverWait(driver, 10).until(EC.visibility_of_element_located((By.XPATH, first_name_input_xpath)))
            first_name_input.clear()
            first_name_input.send_keys("Coca-Cola")
            search_button.click()

            # Wait for the search to complete by confirming the input field becomes empty.
            print("  Waiting for form to clear, confirming 'no results found'.")
            search_cleared = wait.until(EC.text_to_be_present_in_element_value((By.XPATH, first_name_input_xpath), ''))
            
            if search_cleared:
                verify_deletion_test.add_check("Verified form is empty after searching for deleted user", True)
            else:
                # This case should not be hit if the wait works, but it's good practice.
                verify_deletion_test.add_check("User was not deleted, form was not empty", False)

        except Exception as e:
            verify_deletion_test.add_check(f"Verify Deletion test failed: {e}", False)
        all_blocks_passed.append(verify_deletion_test.conclude())

        # This part simulates the end state of test 13 for a clean run of test 14
        wait = WebDriverWait(driver, 10)
        search_button = wait.until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Search']")))
        first_name_input_xpath = "//div[p/span[text()='First Name:']]/following-sibling::div[1]//input"
        first_name_input = wait.until(EC.visibility_of_element_located((By.XPATH, first_name_input_xpath)))
        search_button.click()
        first_name_input.clear()
        first_name_input.send_keys("NoSuchUser")
        search_button.click()
        wait.until(EC.text_to_be_present_in_element_value((By.XPATH, first_name_input_xpath), ''))
        print("--- Setup for Test 14 complete ---\n")
        all_blocks_passed.append(True)

        # TEST 14: Advanced UI Interactions 
        # Time sleep to reduce speed 
        interaction_test = TestBlock(14, "Advanced UI Interactions")
        interaction_test.start()
        try:
            wait = WebDriverWait(driver, 10)
            
            print("  Step 0: Reloading page to guarantee a clean state for this test.")
            driver.get(test_url)
            time.sleep(2)  # Allow time for the page to reload
            wait.until(EC.visibility_of_element_located((By.XPATH, "//span[text()='People']")))
            interaction_test.add_check("Page reloaded for a clean test environment", True)
            time.sleep(2)  # Allow time for the page to fully load
            # Step 1: Search for 'Frank' to set up the interaction test.
            print("  Step 1: Searching for 'Frank' to set up interaction test.")
            search_button = wait.until(EC.element_to_be_clickable((By.XPATH, "//span[text()='Search']")))
            time.sleep(1)  # Ensure the search button is ready
            search_button.click()
            time.sleep(3)
            first_name_input = wait.until(EC.visibility_of_element_located((By.XPATH, first_name_input_xpath)))
            first_name_input.clear()
            time.sleep(1)  # Ensure the input is ready
            first_name_input.send_keys("Frank")
            search_button.click()
            wait.until(EC.text_to_be_present_in_element_value((By.XPATH, first_name_input_xpath), "Frank"))
            interaction_test.add_check("Setup complete, 'Frank' is loaded", True)
            
            # Step 2: Click the first computer row and press TAB
            print("  Step 2: Clicking a table row and tabbing.")
            computer_table_row_xpath = "(//span[text()='Associated Computers']/ancestor::div[1]/following-sibling::div[1]//div[contains(@style, 'overflow: hidden')]/div/div)[1]"
            first_row = wait.until(EC.element_to_be_clickable((By.XPATH, computer_table_row_xpath)))
            ActionChains(driver).click(first_row).pause(0.5).send_keys(Keys.TAB).perform()
            interaction_test.add_check("Clicked row and pressed TAB key", True)

            # Step 3: Click scrollbar arrows
            print("  Step 3: Clicking scrollbar arrows.")
            scrollbar_xpath = "//span[text()='Associated Computers']/ancestor::div[1]/following-sibling::div[2]"
            down_arrow = wait.until(EC.visibility_of_element_located((By.XPATH, f"{scrollbar_xpath}//img[@name='d']")))
            ActionChains(driver).click(down_arrow).pause(0.2).click(down_arrow).perform()
            interaction_test.add_check("Clicked scrollbar down arrows", True)
            
            # Step 4: Drag scrollbar thumb
            print("  Step 4: Dragging scrollbar thumb.")
            scrollbar_thumb = driver.find_element(By.XPATH, f"{scrollbar_xpath}//div[contains(@id, 'box')]")
            ActionChains(driver).drag_and_drop_by_offset(scrollbar_thumb, 0, 50).perform()
            interaction_test.add_check("Dragged scrollbar thumb down", True)

            # Step 5: Drag column resizer
            print("  Step 5: Dragging column resizer.")
            computer_table_pane = "//span[text()='Associated Computers']/ancestor::div[1]/following-sibling::div[1]"
            first_resizer = wait.until(EC.visibility_of_element_located((By.XPATH, f"({computer_table_pane}//div[contains(@style, 'cursor: move')])[1]")))
            ActionChains(driver).drag_and_drop_by_offset(first_resizer, 30, 0).perform()
            interaction_test.add_check("Dragged column resizer to the right", True)
            
        except Exception as e:
            interaction_test.add_check(f"Advanced interaction test failed: {e}", False)
        all_blocks_passed.append(interaction_test.conclude())
        

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        if not all_blocks_passed: all_blocks_passed.append(False)
    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"\nForm + Table Test {final_status}")
        print("---")
        if driver:
            print("Test complete. Browser will close in 10 seconds.")
            time.sleep(10)
            driver.quit()
        sys.exit(0 if final_status == "PASS" else 1)

if __name__ == "__main__":
    run_test()