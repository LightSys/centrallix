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
from test_reporter import TestReporter
reporter = TestReporter("Button")

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

        # Get all buttons by its widget name
        bttn_names = ["textButton", "textButton2", "imgButton", "imgButton2", "topImgButton", "topImgButton2", "rightImgButton", "rightImgButton2", "leftImgButton", "leftImgButton2", "bottomImgButton", "bottomImgButton2"]
        buttons = []
        for bttn in bttn_names:
            buttons.append(driver.execute_script(f"return wgtrFind('{bttn}');"))

        disabled = False
        #pointiimage, tristate
        hoverTestFlags = [True, True]
        clickTestFlag = True
        reporter.add_test(1, "Hover behavior test")
        reporter.add_test(2, "Click behavior test")
        
        for button in buttons:

            ActionChains(driver).move_to_element(button).perform()

            if not disabled:
              # Test1: Hover Behavior
              # 1. pointimage
                try:
                    img = button.find_element(By.XPATH, ".//img").get_attribute("src")
                    if "green" not in img:
                        hoverTestFlags[0] = False
                except:
                    pass
                # 2. tristate
                try: 
                    table = button.find_element(By.XPATH, ".//table")
                    style = button.get_attribute("style")
                    if "border-width: 1px" not in style:
                        hoverTestFlags[1] = False
                except:
                    pass
            
            prev_label = get_label_info(driver)

            # Test clickimage
            ActionChains(driver).click_and_hold(button).perform()
            time.sleep(1)

            # Test connector
            ActionChains(driver).release(button).perform()

            # Verify click result
            if not disabled and prev_label != get_label_info(driver):
              pass
            elif disabled and prev_label == get_label_info(driver):
              pass
            else:
                clickTestFlag = False
            disabled = not disabled
            
            
        reporter.record_check(1, "pointimage change", hoverTestFlags[0])
        reporter.record_check(1, "tristate border change", hoverTestFlags[1])
        
        reporter.record_check(2, "click event updates label", clickTestFlag)

    finally:
        reporter.print_report()
        # Cleanup
        time.sleep(10)
        driver.quit()


if __name__ == "__main__":
    run_test()