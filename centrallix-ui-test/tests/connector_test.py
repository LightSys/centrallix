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
reporter = TestReporter("Connector")


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

def test1(driver):
  reporter.add_test(1, "Window widget events and action test")

  # Check 1
  # Test 5 times at most if status has not updated
  check1_1 = "load event"
  reporter.record_check(1,check1_1,True)

  status = driver.execute_script("return wgtrFind('window_status').content")
  if status != "Window opened":
    count = 0
    while count < 5:
      time.sleep(1)
      status = driver.execute_script("return wgtrFind('window_status').content")
      if status == "Window opened":
        break
      count += 1
    if status != "Window opened":
      reporter.record_check(1,check1_1,False)

  # Check 2
  check1_2 = "close action and close event"
  reporter.record_check(1,check1_2,True)

  ActionChains(driver).click(driver.execute_script("return wgtrFind('closeWindow')")).perform()
  time.sleep(0.1)
  visible = driver.execute_script("return wgtrFind('childwindow').style.visibility")
  status = driver.execute_script("return wgtrFind('window_status').content")

  if visible != "hidden" or status != "Window closed":
    reporter.record_check(1,check1_2,False)

  time.sleep(0.5)

  # Check 3
  check1_3 = "toggle visibility action and open event" 
  reporter.record_check(1,check1_3,True)

  ActionChains(driver).click(driver.execute_script("return wgtrFind('toggleWindow')")).perform()
  time.sleep(0.1)
  status = driver.execute_script("return wgtrFind('window_status').content")
  visible = driver.execute_script("return wgtrFind('childwindow').style.visibility")

  if visible != 'inherit' or status != "Window opened":
    reporter.record_check(1,check1_3,False)
     
def test2(driver):
  reporter.add_test(2, "Component widget events and actions test")

  # Check 1
  # Simply check if component element's component attribute grows in length
  check2_1 = "instantiate action" 
  reporter.record_check(2,check2_1,True)
  cmplen = driver.execute_script("return wgtrFind('component').components.length")
  ActionChains(driver).click(driver.execute_script("return wgtrFind('instantiate_button')")).perform()
  time.sleep(0.5)
  new_cmplen = driver.execute_script("return wgtrFind('component').components.length")

  if new_cmplen <= cmplen:
    reporter.record_check(2,check2_1,False)

  # Check 2
  check2_2 = "LoadComplete event" 
  reporter.record_check(2,check2_2,True)

  time.sleep(0.5)
  status = driver.execute_script("return wgtrFind('cmp_label').content")
  if status != "Load Complete":
    reporter.record_check(2,check2_2,False)

  # Check 3
  check2_3 = "Destroy action" 
  reporter.record_check(2,check2_3,True)

  ActionChains(driver).click(driver.execute_script("return wgtrFind('destroy_button')")).perform()
  time.sleep(0.5)
  cmplen = driver.execute_script("return wgtrFind('component').components.length")
  if cmplen != 0:
    reporter.record_check(2,check2_3,False)

def test3(driver):
  reporter.add_test(3, "Datetime widget events and actions test")
  datetime = driver.execute_script("return wgtrFind('datetime')")

  # Check 1
  check3_1 = "GetFocus event" 
  reporter.record_check(3,check3_1,True)
  ActionChains(driver).click(datetime).perform()
  time.sleep(0.3)
  status =  driver.execute_script("return wgtrFind('dtstatus_label').content")
  if status != "Got focus":
    reporter.record_check(3,check3_1,False)
  
  # Check 2
  check3_2 = "DataChange event" 
  reporter.record_check(3,check3_2,True)
  ActionChains(driver).send_keys("t").key_down(Keys.ENTER).perform()
  time.sleep(0.1)
  status =  driver.execute_script("return wgtrFind('dtstatus_label').content")
  if status != "Data change":
    reporter.record_check(3,check3_2,False)

  # Check 3
  check3_3 = "LoseFocus event" 
  reporter.record_check(3,check3_3,True)
  ActionChains(driver).click(datetime).perform()
  time.sleep(0.1)
  ActionChains(driver).move_to_element_with_offset(datetime,0,20).click().perform()
  time.sleep(0.1)
  status =  driver.execute_script("return wgtrFind('dtstatus_label').content")
  if status != "Lost focus":
    reporter.record_check(3,check3_3,False)

def test4(driver):
  reporter.add_test(4, "Dropdown widget events and actions test")
  dropdown = driver.execute_script("return wgtrFind('dropdown')")

  # Check 1
  check4_1 = "GetFocus event" 
  reporter.record_check(4,check4_1,True)
  ActionChains(driver).click(dropdown).perform()
  time.sleep(0.3)
  status =  driver.execute_script("return wgtrFind('ddstatus_label').content")
  if status != "Got focus":
    reporter.record_check(4,check4_1,False)

  # Check 2
  check4_2 = "DataChange event" 
  reporter.record_check(4,check4_2,True)
  ActionChains(driver).key_down(Keys.ENTER).perform()
  time.sleep(0.1)
  status =  driver.execute_script("return wgtrFind('ddstatus_label').content")
  if status != "Data change":
    reporter.record_check(4,check4_2,False)

  # Check 3
  check4_3 = "LoseFocus event" 
  reporter.record_check(4,check4_3,True)
  ActionChains(driver).click(dropdown).perform()
  ActionChains(driver).move_to_element_with_offset(dropdown,0,-20).click().perform()
  time.sleep(0.1)
  status =  driver.execute_script("return wgtrFind('ddstatus_label').content")
  if status != "Lost focus":
    reporter.record_check(4,check4_3,False)

  # Check 4
  check4_4 = "SetItems action" 
  reporter.record_check(4,check4_4,True)
  
  ActionChains(driver).click(driver.execute_script("return wgtrFind('setitems_btn')")).perform()
  ActionChains(driver).click(dropdown).perform()
  time.sleep(0.3)
  # Hard coded check - varifies if dropdown has more that 2 items(default # of dropdown items) after setItems
  if driver.execute_script("return arguments[0].Items.length",dropdown) < 3:
    reporter.record_check(4,check4_4,False)

  # Check 5
  check4_5 = "SetGroup action"
  reporter.record_check(4,check4_5,True)

  prevItemNum = driver.execute_script("return arguments[0].Items.length",dropdown)
  ActionChains(driver).click(driver.execute_script("return wgtrFind('setgroup_btn')")).perform()
  ActionChains(driver).click(dropdown).perform()
  time.sleep(0.3)
  currentItemNum = driver.execute_script("return arguments[0].Items.length",dropdown)
  # Return false if dropdown has more items than before clicking setGroup 
  # because the # of grouped items will be less or equal to whole # of item
  if currentItemNum > prevItemNum:
    reporter.record_check(4,check4_5,False)

def test5(driver):
  reporter.add_test(5, "Editbox widget events and actions test")
  editbox = driver.execute_script("return wgtrFind('editbox')")

  # Check 1
  check5_1 = "GetFocus event" 
  reporter.record_check(5,check5_1,True)
  ActionChains(driver).click(editbox).perform()
  time.sleep(0.3)
  status =  driver.execute_script("return wgtrFind('editbox_status_label').content")
  if status != "Got focus":
    reporter.record_check(5,check5_1,False)
  
  # Check 2
  check5_2 = "LoseFocus event" 
  reporter.record_check(5,check5_2,True)
  ActionChains(driver).send_keys("t").key_down(Keys.ENTER).perform()
  time.sleep(0.1)
  status =  driver.execute_script("return wgtrFind('editbox_status_label').content")
  if status != "Lost focus":
    reporter.record_check(5,check5_2,False)

  # Check 3
  check5_3 = "SetValue event" 
  reporter.record_check(5,check5_3,True)
  setvalue_btn = driver.execute_script("return wgtrFind('setvalue_btn')")
  ActionChains(driver).click(setvalue_btn).perform()
  time.sleep(0.1)
  value = driver.execute_script("return arguments[0].value",editbox)
  if value != 'New Value':
    reporter.record_check(5,check5_3,False)

  # Check 4
  check5_4 = "SetValueDescription"
  reporter.record_check(5,check5_4,True)
  setdesc_btn = driver.execute_script("return wgtrFind('setvaluedescription_btn')")
  ActionChains(driver).click(setdesc_btn).perform()
  time.sleep(0.1)
  value = driver.execute_script("return arguments[0].description",editbox)
  if value != 'New Descrip':
    reporter.record_check(5,check5_4,False)

# def test6(driver): form test later..?

def test7(driver):
  reporter.add_test(7, "HTML load action test")
  check7_1 = "LoadPage action"
  reporter.record_check(7,check7_1,True)
  html = driver.execute_script("return wgtrFind('html_html')")
  loadbtn = driver.execute_script("return wgtrFind('html_loadpage')")
  ActionChains(driver).click(loadbtn).perform()
  if driver.execute_script("return arguments[0].style.visibility",html) != "hidden":
    reporter.record_check(7,check7_1,False)
  

def test8(driver):
  reporter.add_test(8, "Textarea widget events and actions test")
  textarea = driver.execute_script("return wgtrFind('textarea_textarea')")

  # Check 1
  check8_1 = "GetFocus event" 
  reporter.record_check(8,check8_1,True)
  ActionChains(driver).click(textarea).perform()
  time.sleep(0.3)
  status =  driver.execute_script("return wgtrFind('textarea_status_label').content")
  if status != "Got focus":
    reporter.record_check(8,check8_1,False)
  
  # Check 2
  check8_2 = "LoseFocus event" 
  reporter.record_check(8,check8_2,True)
  ActionChains(driver).move_to_element_with_offset(textarea,0,-30).click().perform()
  time.sleep(0.1)
  status =  driver.execute_script("return wgtrFind('textarea_status_label').content")
  if status != "Lost focus":
    reporter.record_check(8,check8_2,False)

def test9(driver):
  reporter.add_test(9, "Textbutton widget events and actions test")
  txtbtn = driver.execute_script("return wgtrFind('textbutton_textbutton')")

  # Check 1
  check9_1 = "Click event" 
  reporter.record_check(9,check9_1,True)
  ActionChains(driver).click(txtbtn).perform()
  status =  driver.execute_script("return wgtrFind('textbutton_status_label').content")
  if status != "clicked":
    reporter.record_check(9,check9_1,False)

  # Check 2
  check9_2 = "SetText action" 
  reporter.record_check(9,check9_2,True)
  settextbtn = driver.execute_script("return wgtrFind('textbutton_setTextButton')")
  ActionChains(driver).click(settextbtn).perform()

  if 'new text' not in driver.execute_script("return arguments[0].innerHTML",txtbtn):
    reporter.record_check(9,check9_2,False)
  





def run_test():

  try:
      config = toml.load("config.toml")
  except FileNotFoundError:
      print("Config.toml is missing. Make sure to rename config.template and try again.")
      return

  test_url = config["url"] + "/tests/ui/connector/connector_test.app"
  driver = create_driver(test_url)
  time.sleep(1)

  test1(driver)
  test2(driver)
  test3(driver)
  test4(driver)
  test5(driver)
  test7(driver)
  test8(driver)
  test9(driver)

  result = reporter.print_report()
  time.sleep(5)
  driver.quit()
  sys.exit(0) if result else sys.exit(1)

            
if __name__ == "__main__":
  run_test()