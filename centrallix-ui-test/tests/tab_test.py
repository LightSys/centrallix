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
reporter = TestReporter("Tab")


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

def run_test():
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print("Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/tab_test.app"
    driver = create_driver(test_url)

    try:
        # Wait for dropdowns to appear
        WebDriverWait(driver, 10).until(EC.presence_of_all_elements_located((By.XPATH, "//div[contains(@id, 'tc')]")))

        # Get tab widgets
        tab_widget_names = ["tab1", "tab2", "tab3", "tab4", "tab5"]
        tab_elems = []
        for name in tab_widget_names:
            tab_elems.append(driver.execute_script(f"return wgtrFind('{name}')"))

        for tab in tab_elems:
            if tab == None:
                raise ValueError("Dropdown element not found")
                sys.exit(1)

        # Test 1
        reporter.add_test(1, "Tab page click behavior test")
        check1_1 = "current tabpage change"
        reporter.record_check(1,check1_1,True)
        
        for tab in tab_elems:
          tab_pages = driver.execute_script(f"return arguments[0].tabs",tab)
          prev_tab = driver.execute_script(f"return arguments[0].current_tab",tab)
          for i in range(len(tab_pages)):
            tab = driver.execute_script(f"return arguments[0].tab",tab_pages[i])
            ActionChains(driver).click(tab).perform()
            current_tab = driver.execute_script(f"return arguments[0].current_tab",tab)
            if prev_tab == current_tab:
              reporter.record_check(1,check1_1,False)
            
    finally:
        result = reporter.print_report()
        time.sleep(5)
        driver.quit()
        sys.exit(0) if result else sys.exit(1)


if __name__ == "__main__":
    run_test()
