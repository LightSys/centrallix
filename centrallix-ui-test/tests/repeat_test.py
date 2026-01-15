# Minsik Lee May 2025

"""Notes"""
# -

""" Module allowing web testing using pure Selenium """

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
reporter = TestReporter("Repeat")


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

  test_url = config["url"] + "/tests/ui/repeat/repeat_test.app"
  driver = create_driver(test_url)
  
  
  buttonWidget = "btn"
  buttons = driver.execute_script(f"return wgtrFind('{buttonWidget}').nodelist")
  
  # Test 1
  reporter.add_test(1, "Component click action and event behavior test")
  check1_1 = "Component event"
  check1_2 = "Component action"
  reporter.record_check(1,check1_1,True)
  reporter.record_check(1,check1_2,True)
    
  for button in buttons:
    label = f"Clicked {driver.execute_script("return arguments[0].buttonText",button)}.csv"
    ActionChains(driver).click(button).perform()
    
    time.sleep(0.5)
    # instantiated components, get the last element to get the latest instance
    cmp_wgts = driver.execute_script("return wgtrFind('cmp').components.slice(-1)[0].objects[0].__WgtrChildren")

    for i in range(len(cmp_wgts)):
      if driver.execute_script("return wgtrGetType(arguments[0])",cmp_wgts[i]) == "widget/button":
        ActionChains(driver).click(cmp_wgts[i]).perform()
        if label != driver.execute_script("return wgtrFind('label').content"):
          reporter.record_check(1,check1_1,False)
          
    ActionChains(driver).click(driver.execute_script("return wgtrFind('update_button')")).perform()
    for i in range(len(cmp_wgts)):
      if driver.execute_script("return wgtrGetType(arguments[0])",cmp_wgts[i]) == "widget/label":
        if driver.execute_script("return arguments[0].content",cmp_wgts[i]) != "New Value":
          reporter.record_check(1,check1_2,False)
          
  result = reporter.print_report()
  time.sleep(5)
  driver.quit()
  sys.exit(0) if result else sys.exit(1)
          
if __name__ == "__main__":
  run_test()
