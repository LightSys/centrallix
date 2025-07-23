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
reporter = TestReporter("Table")


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
  
def test1(driver, table):
  reporter.add_test(1, "Table row click and click event behavior test")
  check1_1 = "selected row change"
  check1_2 = "click event bahvior"
  check1_3 = "double click event bahvior"
  reporter.record_check(1,check1_1,True)
  reporter.record_check(1,check1_2,True)
  reporter.record_check(1,check1_3,True)
  
  table_rows = driver.execute_script(f"return arguments[0].rows", table)
  
  repeat_range = 11 if len(table_rows) > 11 else len(table_rows)
  prev_selected, prev_lbl = 1, "Tabel Label"
  for i in range(2, repeat_range):
    row = driver.execute_script(f"return arguments[0][{i}]", table_rows)
    ActionChains(driver).click(row).perform()
    time.sleep(0.5)
    
    # row select check
    new_selected = driver.execute_script(f"return arguments[0].selected", table)
    if new_selected == prev_selected:
      reporter.record_check(1,check1_1,False)
    prev_selected = new_selected
    
    # click event check
    new_lbl = driver.execute_script(f"return wgtrFind('lbl').content")
    if new_lbl == prev_lbl:
      reporter.record_check(1,check1_2,False)
    prev_lbl = new_lbl
    
    # double click event check
    ActionChains(driver).double_click(row).perform()
    time.sleep(0.1)
    new_lbl = driver.execute_script(f"return wgtrFind('lbl').content")
    if new_lbl == prev_lbl:
      reporter.record_check(1,check1_3,False)
      
def scroll(driver, table, btn, type):
  prev_pos = "".join(c for c in driver.execute_script(f"return arguments[0].scrolldiv.style.top", table) if c.isdigit())
  if type == "button":
    ActionChains(driver).click(btn).perform()
  elif type == "bar":
    ActionChains(driver).click_and_hold(btn).move_by_offset(0,-10).perform()
  else:
    return False
  time.sleep(0.1)
  new_pos = "".join(c for c in driver.execute_script(f"return arguments[0].scrolldiv.style.top", table) if c.isdigit())
  print(f"prev:{prev_pos} new:{new_pos}")

  # click scroll button until scrolldiv top attribute doesn't update
  while new_pos != prev_pos:
    prev_pos = new_pos
    if type == "button":
      ActionChains(driver).click(btn).perform()
    elif type == "bar":
      ActionChains(driver).click_and_hold(btn).move_by_offset(0,-10).perform()
    else: 
      print("wrong type: neither button or bar")
      break
    new_pos = "".join(c for c in driver.execute_script(f"return arguments[0].scrolldiv.style.top", table) if c.isdigit())
    
def check_scroll_result(driver, table, check):
  if driver.execute_script(f"return arguments[0].rows.last", table) == None:
    reporter.record_check(2,check,False)
    # Error: no row visible after scroll
    return False
  else:
    try:
      lastvis = driver.execute_script(f"return arguments[0].rows.lastvis", table)
      visible = driver.execute_script(f"return arguments[0].IsRowVisible({lastvis})", table)
      if visible != "full" and visible != "partial":
        reporter.record_check(2,check,False)
    except:
      # Error: no row visible after scroll
      return False
  return True
  
      
def test2(driver, table):
  reporter.add_test(2, "Scroll behavior test")
  check2_1 = "scroll up and down button behavior"
  check2_2 = "scroll bar behavior"
  reporter.record_check(2,check2_1,True)
  reporter.record_check(2,check2_2,True)
  
  down_btn = driver.execute_script(f"return arguments[0].down", table)
  scroll(driver, table, down_btn, "button")
  scroll_down_result = check_scroll_result(driver, table, check2_1)

  if scroll_down_result:
    up_btn = driver.execute_script(f"return arguments[0].up", table) 
    scroll(driver, table, up_btn)
    check_scroll_result(driver, table, check2_1)

    scrollbar = driver.execute_script(f"return arguments[0].scrollbar.b", table) 
    scroll(driver, table, scrollbar, "bar")
    check_scroll_result(driver, table, check2_2)
    
  else:
    reporter.record_check(2,check2_2,False) # All row invisible - can not test scrollbar
      
def run_test():

  try:
      config = toml.load("config.toml")
  except FileNotFoundError:
      print("Config.toml is missing. Make sure to rename config.template and try again.")
      return

  test_url = config["url"] + "/tests/ui/table/table_test.app"
  driver = create_driver(test_url)
  
  table = driver.execute_script(f"return wgtrFind('table')")
  
  test1(driver, table)
  test2(driver, table)
  
  result = reporter.print_report()
  time.sleep(5)
  driver.quit()
  sys.exit(0) if result else sys.exit(1)
          
if __name__ == "__main__":
  run_test()
