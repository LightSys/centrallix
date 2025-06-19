# Minsik Lee May 2025

import toml
import time
import sys
import re
from selenium import webdriver
from selenium.webdriver import ActionChains, Keys
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager
from test_reporter import TestReporter
reporter = TestReporter("Formstatus")


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

def is_formstatus(driver, fs, status):
  return status == driver.execute_script(f"return arguments[0].currentMode", fs)
   
  
def run_test():

  try:
      config = toml.load("config.toml")
  except FileNotFoundError:
      print("Config.toml is missing. Make sure to rename config.template and try again.")
      return

  test_url = config["url"] + "/tests/ui/formstatus_test.app"
  driver = create_driver(test_url)

  time.sleep(1)
  
  fs = driver.execute_script(f"return wgtrFind('lg_formstatus')")
  editbox = driver.execute_script(f"return wgtrFind('editbox')")
  savebtn = driver.execute_script(f"return wgtrFind('save_btn')")
  searchbtn = driver.execute_script(f"return wgtrFind('searchbtn')")

  reporter.add_test(1, "Form status update test")
  check1_1 = "No Data"
  check1_2 = "New"
  check1_3 = "View"
  check1_4 = "Search"
  check1_5 = "Searching"
  reporter.record_check(1,check1_1,True)
  reporter.record_check(1,check1_2,True)
  reporter.record_check(1,check1_3,True)
  reporter.record_check(1,check1_4,True)
  reporter.record_check(1,check1_5,True)

  # Check initial status: No data
  reporter.record_check(1,check1_1,is_formstatus(driver, fs, "NoData"))

  # Click edit box and enter new state
  ActionChains(driver).click(editbox).perform()
  reporter.record_check(1,check1_2,is_formstatus(driver, fs, "New"))

  # Save and enter view state
  ActionChains(driver).send_keys("a").perform()
  time.sleep(0.5)
  ActionChains(driver).click(savebtn).perform()
  reporter.record_check(1,check1_3,is_formstatus(driver, fs, "View"))

  # Click search button and enter search state
  ActionChains(driver).click(searchbtn).perform()
  time.sleep(0.5)
  reporter.record_check(1,check1_4,is_formstatus(driver, fs, "Search"))

  # Click search button again and enter query execute state
  ActionChains(driver).send_keys("a").perform()
  time.sleep(0.5)
  ActionChains(driver).click(searchbtn).perform()
  time.sleep(0.5)
  reporter.record_check(1,check1_4,is_formstatus(driver, fs, "QueryExec"))


  result = reporter.print_report()
  time.sleep(5)
  driver.quit()
  sys.exit(0) if result else sys.exit(1)
          
if __name__ == "__main__":
  run_test()
