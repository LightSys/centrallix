# Minsik Lee May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

"""Notes"""
# - 

""" Module allowing web testing using pure Selenium """

import toml
import time
from selenium import webdriver
from selenium.webdriver import ActionChains
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager

def create_driver(test_url) -> webdriver.Chrome:
    """Create and return a configured Chrome WebDriver."""
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_options.add_argument('--ignore-certificate-errors')  # Skip Certificatgite/by pass it 

    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)

    # Wait until the page has fully loaded
    WebDriverWait(driver, 10).until(
        lambda d: d.execute_script("return document.readyState") == "complete"
    )

    return driver

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
        # Wait for label to appear
        WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located((By.XPATH, "//div[contains(@id, 'dd')]"))
        )

        # Get dropdown widgets
        prefix = "dd"
        suffix = "btn"
        xpath_expression = (
            f"//div[starts-with(@id, '{prefix}') and "
            f"substring(@id, string-length(@id) - string-length('{suffix}') + 1) = '{suffix}' and "
            f"number(substring(@id, string-length('{prefix}') + 1, string-length(@id) - string-length('{prefix}') - string-length('{suffix}'))) = "
            f"number(substring(@id, string-length('{prefix}') + 1, string-length(@id) - string-length('{prefix}') - string-length('{suffix}')))]"
        )

        dropdowns = WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located((By.XPATH, xpath_expression))
        )

        # Dropdown test
        for dropdown in dropdowns:
          ddid = dropdown.get_attribute("id")
          number = ''.join([c for c in ddid if c.isdigit()])
          print(f"Testing: {ddid}")
          print("[TEST 1: Dropdown item click]")

          # 1. Expand dropdown items
          print("1_1 Click dropdown and expand ...")
          ActionChains(driver).click(dropdown).perform()

          # 2. Click dropdown item
          print("1_2 Click dropdown item ...")
          ddContainerElem = driver.find_element(By.CSS_SELECTOR, "[style*='position: absolute; visibility: inherit; background-color: rgb(192, 192, 192); z-index: 2000;']")
          containerSubelems = ddContainerElem.find_elements(By.XPATH, "./div")
          dropdownElems = containerSubelems[0].find_elements(By.XPATH, ".//*")

          selectedItem = ""

          # Click items five times
          for i in range(5):
            clickItem = dropdownElems[i+2].text
            ActionChains(driver).click(dropdownElems[i+2]).perform()
            print(f"    clicked: {clickItem}")

            # 3. Check if selected item updates
            if i%2 == 0:
                # con2 updated 
                con2 = dropdown.find_element(By.ID, f"dd{number}con2")
                con2text = con2.find_element(By.XPATH, ".//td")
                selectedItem = con2text.text
            else:
                # con1 updated
                con1 = dropdown.find_element(By.ID, f"dd{number}con1")
                con1text = con1.find_element(By.XPATH, ".//td")
                selectedItem = con1text.text

            if selectedItem == clickItem:
                print(f"    PASS: {clickItem} successfully selected\n")
            else: 
                print(f"    FAIL: drop down item select failed (clicked {clickItem} but {selectedItem} selected)\n")
                break
            ActionChains(driver).click(dropdown).perform()

          print("[TEST 2: Scroll]")
          # 1. move scroll bar
          print("2_1 Drag scrollbar down ...")

          scrollBarElem = containerSubelems[2]
          containerStyle = containerSubelems[0].get_attribute("style").split(";")[2]
          initialTopVal = int(''.join([c for c in containerStyle if c.isdigit()]))

          for _ in range(3):
            ActionChains(driver).move_to_element(scrollBarElem).click_and_hold().move_by_offset(0, 20).pause(0.1).release().perform()
            containerStyle = containerSubelems[0].get_attribute("style").split(";")[2]
            containerTopVal = int(''.join([c for c in containerStyle if c.isdigit()]))
            
            if containerTopVal < initialTopVal:
                print("    FAIL: scroll down")
                break
            initialTopVal = containerTopVal
          ActionChains(driver).release().perform()

          print("2_2 Drag scrollbar up ...")

          containerStyle = containerSubelems[0].get_attribute("style").split(";")[2]
          initialTopVal = int(''.join([c for c in containerStyle if c.isdigit()]))

          for _ in range(3):
            ActionChains(driver).move_to_element(scrollBarElem).click_and_hold().move_by_offset(0, -20).pause(0.1).release().perform()
            containerStyle = containerSubelems[0].get_attribute("style").split(";")[2]
            containerTopVal = int(''.join([c for c in containerStyle if c.isdigit()]))
            if containerTopVal > initialTopVal:
                print("    FAIL: scroll down")
                break
            initialTopVal = containerTopVal
          ActionChains(driver).release().perform()

          # 2. Click scroll button
          scrollBtns = containerSubelems[1].find_elements(By.XPATH, "./table//img")
          scrollUpBtn = scrollBtns[0]
          scrollDownBtn = scrollBtns[2]
          
          print("2_3 Click scroll down button ...")
          initContainerStyle = containerSubelems[0].get_attribute("style").split(";")[2]
          initContainerTopVal = int(''.join([c for c in initContainerStyle if c.isdigit()]))

          ActionChains(driver).click(scrollDownBtn).perform()

          containerStyle = containerSubelems[0].get_attribute("style").split(";")[2]
          containerTopVal = int(''.join([c for c in containerStyle if c.isdigit()]))

          if containerTopVal < initContainerTopVal:
              print("    FAIL: scroll down button")
          else: 
              print("    PASS: scroll down button")

          print("2_4 Click scroll up button ...")
          initContainerStyle = containerSubelems[0].get_attribute("style").split(";")[2]
          initContainerTopVal = int(''.join([c for c in initContainerStyle if c.isdigit()]))

          ActionChains(driver).click(scrollUpBtn).perform()

          containerStyle = containerSubelems[0].get_attribute("style").split(";")[2]
          containerTopVal = int(''.join([c for c in containerStyle if c.isdigit()]))

          if containerTopVal > initContainerTopVal:
              print("    FAIL: scroll up button\n")
          else: 
              print("    PASS: scroll up button\n")

          ActionChains(driver).click(dropdown).perform()



    finally:
        # Cleanup
        time.sleep(10)
        driver.quit()


if __name__ == "__main__":
    run_test()