# Minsik Lee May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

"""Notes"""

""" Module allowing web testing using pure Selenium """

import toml
import time
import sys
from selenium import webdriver
from selenium.webdriver import ActionChains
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager
from test_reporter import TestReporter
reporter = TestReporter("Menu")


def create_driver(test_url) -> webdriver.Chrome:
    """Create and return a configured Chrome WebDriver."""
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    # Skip Certificatgite/by pass it
    chrome_options.add_argument('--ignore-certificate-errors')

    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)

    # Wait until the page has fully loaded
    WebDriverWait(driver, 10).until(
        lambda d: d.execute_script("return document.readyState") == "complete"
    )

    return driver


def get_label_info(driver: webdriver.Chrome):
    """Return status of the label."""
    try:
        label_element = driver.find_element(By.XPATH, "//div[contains(@id, 'lbl')]//span").text
        return (label_element)
    except Exception as e:
        print(f"Error retrieving label properties: {e}")


def run_test():
    """Run menu test."""
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print("Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/menu_test.app"
    driver = create_driver(test_url)
    action = ActionChains(driver)

    try:
        # Wait for menu to appear
        WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located(
                (By.XPATH, "//div[contains(@id, 'mn')]"))
        )

        # Get main menu element
        prefix = "mn"
        suffix = "main"
        xpath_expression = (
            f"//div[starts-with(@id, '{prefix}') and "
            f"substring(@id, string-length(@id) - string-length('{suffix}') + 1) = '{suffix}' and "
            f"number(substring(@id, string-length('{prefix}') + 1, string-length(@id) - string-length('{prefix}') - string-length('{suffix}'))) = "
            f"number(substring(@id, string-length('{prefix}') + 1, string-length(@id) - string-length('{prefix}') - string-length('{suffix}')))]"
        )
        main_menu_elems = driver.find_elements(By.XPATH, xpath_expression)

        # TEST1
        reporter.add_test(1, "Mouse over")
        check1_1 = "mouse over behavior"
        # - Test mouse over functionality. For menu expand to display menu items
        # get "mn##high" div that changes its syle when mouse hover
        reporter.record_check(1, check1_1, True)
        high_div = main_menu_elems[0].find_element(
            By.XPATH, "./div[contains(@id, 'high')]")
        init_high_style = 0

        menu_items_elems = main_menu_elems[0].find_elements(
            By.XPATH, ".//td[(@nowrap and @valign = 'middle')]")

        # Hover mouse over menu items
        for i in range(len(menu_items_elems)):
            action.move_to_element(menu_items_elems[i]).pause(1.5).perform()

            high_style = int(
                ''.join(filter(str.isdigit, high_div.get_attribute("style").split(";")[3])))
            if high_style < init_high_style:
                reporter.record_check(1, check1_1, False)

            updated_main_menu_elems = driver.find_elements(By.XPATH, xpath_expression)
            init_high_style = high_style
            main_menu_elems = updated_main_menu_elems

        # TEST2: Click a Menu Item
        reporter.add_test(2, "Click menu items")
        # - Test if menu items close after click
        # - Test if select event is triggered properly
        check2_1 = "click behavior test"
        reporter.record_check(2, check2_1, True)
        
        # Child window element XPATH
        prefix = "wn"
        suffix = "base"
        xpath_expression = (
            f"//div[starts-with(@id, '{prefix}') and "
            f"substring(@id, string-length(@id) - string-length('{suffix}') + 1) = '{suffix}' and "
            f"number(substring(@id, string-length('{prefix}') + 1, string-length(@id) - string-length('{prefix}') - string-length('{suffix}'))) = "
            f"number(substring(@id, string-length('{prefix}') + 1, string-length(@id) - string-length('{prefix}') - string-length('{suffix}')))]"
        )

        # Iterate through main menu elements
        for idx in range(len(menu_items_elems)):
            effectFlag = False
            action.click(menu_items_elems[idx]).perform()
            item_name = menu_items_elems[idx].text
            
            # Test sub menu items
            sub_menu_idx = -1

            for i in range(len(main_menu_elems)):
              if "visibility: inherit" in main_menu_elems[i].get_attribute("style"):
                  sub_menu_idx = i
                  break

            if sub_menu_idx != -1:
                sub_menu_items = main_menu_elems[sub_menu_idx].find_elements(
                    By.XPATH, ".//td[(@nowrap and @valign = 'middle')]")

                for item in sub_menu_items:
                    flag = False
                    item_name = item.text
                    label_before = get_label_info(driver)

                    action.click(item).perform()
                    time.sleep(1)

                    # If opened a child window
                    child_window_elem = driver.find_element(By.XPATH, xpath_expression)
                    child_window_state = child_window_elem.get_attribute("style")
                    if "visibility: inherit" in child_window_state:
                        flag = True
                        action.click(child_window_elem.find_element(By.XPATH, ".//img[contains(@name, 'close')]")).perform()

                    # If triggered select event
                    if label_before != get_label_info(driver):
                        flag = True

                    if flag == False:
                        reporter.record_check(2,check2_1,False)

                    action.click(menu_items_elems[idx]).perform()
            else:
                #If triggered select event
                if label_before != get_label_info(driver):
                    effectFlag = True
                if effectFlag == False:
                    reporter.record_check(2,check2_1,False)

        # TEST3: Right Click Popup
        reporter.add_test(3, "Right click popup")
        check3_1 = "Right click popup"
        reporter.record_check(3, check3_1, False)
        
        label_element = driver.find_element(
            By.XPATH, "//div[contains(@id, 'lbl')]//span")
        action.move_to_element_with_offset(label_element, 30, 30).context_click().perform()
        for i in range(len(main_menu_elems)):
                if "visibility: inherit" in main_menu_elems[i].get_attribute("style"):
                    reporter.record_check(3, check3_1, True)

    finally:
        result = reporter.print_report()
        time.sleep(5)
        driver.quit()
        sys.exit(0) if result else sys.exit(1)

if __name__ == "__main__":
    run_test()
