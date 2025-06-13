# Minsik Lee May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

"""Notes"""
# -

""" Module allowing web testing using pure Selenium """

import toml
import time
import sys
import re
from selenium import webdriver
from selenium.webdriver import ActionChains
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager
from test_reporter import TestReporter
reporter = TestReporter("Dropdown")


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

def get_number(s):
    match = re.match(r"rect\(\s*\d+px,\s*\d+px,\s*(\d+)px,", s)
    if match:
        return int(match.group(1))
    else:
        return 0


def run_test():
    """Run the check box test."""
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print("Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/dropdown_test.app"
    driver = create_driver(test_url)

    try:
        # Wait for dropdowns to appear
        WebDriverWait(driver, 10).until(EC.presence_of_all_elements_located((By.XPATH, "//div[contains(@id, 'dd')]")))

        # Get dropdown widgets
        dd_widget_names = ["Dropdown1", "Dropdown2"]
        dd_elems = []
        for name in dd_widget_names:
            dd_elems.append(driver.execute_script(f"return wgtrFind('{name}')"))

        for dd in dd_elems:
            if dd == None:
                raise ValueError("Dropdown element not found")
                sys.exit(1)

        # Test 1
        reporter.add_test(1, "Dropdown item click behavior test")
        check1_1 = "value change"
        reporter.record_check(1,check1_1,True)
        
        for dropdown in dd_elems:
            ActionChains(driver).click(dropdown).perform()

            # Click items
            items = driver.execute_script(f"return arguments[0].PaneLayer.ScrLayer.childNodes", dropdown)
            downbtn = driver.execute_script(f"return arguments[0].imgdn", dropdown)    
            height = int(''.join(s for s in driver.execute_script(f"return arguments[0].PaneLayer.ScrLayer.childNodes[0].style.height", dropdown) if s.isdigit()))

            for i in range(len(items)):
                if i > 0:
                    ActionChains(driver).click(dropdown).perform()
                
                container_clip = driver.execute_script(f"return arguments[0].PaneLayer.ScrLayer.style.clip", dropdown)
                containerBottom = get_number(container_clip)
                top = int(''.join(s for s in driver.execute_script(f"return arguments[0].PaneLayer.ScrLayer.childNodes[{i}].style.top", dropdown) if s.isdigit()))
                visible = top + height <= containerBottom
                old_val = driver.execute_script(f"return arguments[0].value", dropdown)
                
                moved_down = False
                while(not visible):
                    moved_down = True
                    ActionChains(driver).click(downbtn).perform()
                    container_clip = driver.execute_script(f"return arguments[0].PaneLayer.ScrLayer.style.clip", dropdown)
                    containerBottom = get_number(container_clip)
                    visible = top + height <= containerBottom
                if moved_down:
                    ActionChains(driver).move_to_element(downbtn).move_by_offset(-10,0).click().perform()
                else:
                    ActionChains(driver).move_to_element(items[i]).perform()


                ActionChains(driver).click(items[i]).perform()
                
                new_val = driver.execute_script(f"return arguments[0].value", dropdown)
                if i > 0 and old_val == new_val:
                    reporter.record_check(1,check1_1,False)

    finally:
        result = reporter.print_report()
        time.sleep(5)
        driver.quit()
        sys.exit(0) if result else sys.exit(1)


if __name__ == "__main__":
    run_test()
