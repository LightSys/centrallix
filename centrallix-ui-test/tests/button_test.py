# Minsik Lee May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

"""Notes"""
# -  textoverImgButton seems to be not working and 
#    other buttons stop working when added on the page.
# -  Button with text type shows the clickimage only when clicked,
#    but the rest of the buttons show both image and clickimage when clicked.

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

def get_label_info(driver: webdriver.Chrome):
    """Return status of the label."""
    try:
        label_element = driver.find_element(By.XPATH, "//div[contains(@id, 'lbl')]//span").text
        return(label_element)
    except Exception as e:
        print(f"Error retrieving label properties: {e}")

def run_test():
    """Run the check box test."""
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print("Config.toml is missing. Make sure to rename config.template and try again.")
        return

    test_url = config["url"] + "/tests/ui/button_test.app"
    driver = create_driver(test_url)

    try:
        # Wait for label to appear
        WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located((By.XPATH, "//div[contains(@id, 'lbl')]"))
        )

        # Initial read
        print(f"Initial label text: {get_label_info(driver)}\n")

        # Get buttons
        prefix = "gb"
        suffix = "pane"
        # gb1234pane
        xpath_expression = (
            f"//div[starts-with(@id, '{prefix}') and "
            f"substring(@id, string-length(@id) - string-length('{suffix}') + 1) = '{suffix}' and "
            f"number(substring(@id, string-length('{prefix}') + 1, string-length(@id) - string-length('{prefix}') - string-length('{suffix}'))) = "
            f"number(substring(@id, string-length('{prefix}') + 1, string-length(@id) - string-length('{prefix}') - string-length('{suffix}')))]"
        )
        buttons = WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located((By.XPATH, xpath_expression))
        )

        disabled = False
        for button in buttons:

            # Get button name
            try:
                button_name = button.find_element(By.XPATH, ".//b").text
                print(f"testing {button_name}")
            except:
                msg = "tesing image button" if not disabled else "testing disabled image button"
                print(msg)

            ActionChains(driver).move_to_element(button).perform()

            if not disabled:
              # Test Hover
              # 1. pointimage
              try:
                  img = button.find_element(By.XPATH, ".//img").get_attribute("src")
                  if "green" in img:
                    print("hover test pass")
              except:
                  pass
              # 2. tristate
              try:
                  style = button.get_attribute("style")
                  # print(style)
                  if "border-width: 1px" in style:
                      print("tristate test pass")
              except:
                  pass
            
            prev_label = get_label_info(driver)

            # Test clickimage
            ActionChains(driver).click_and_hold(button).perform()
            time.sleep(1)

            # Test connector
            ActionChains(driver).release(button).perform()

            # Print click result
            # print(f"click result: {get_label_info(driver)}\n")
            if not disabled and prev_label != get_label_info(driver):
              print("Test PASS\n")
            elif disabled and prev_label == get_label_info(driver):
              print("Test PASS\n")
            else:
                print("Test FAILED\n")
            disabled = not disabled

    finally:
        # Cleanup
        time.sleep(10)
        driver.quit()


if __name__ == "__main__":
    run_test()