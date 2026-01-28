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
import sys
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
        label_text = driver.find_element(By.XPATH, "//div[contains(@id, 'lbl')]//span").text
        return(label_text)
    except Exception as e:
        print(f"Error retrieving label properties: {e}")

def run_test():
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

        # Get all buttons by the widget name
        bttn_names = ["textButton", "textButton2", "imgButton", "imgButton2", "topImgButton", "topImgButton2", "rightImgButton", "rightImgButton2", "leftImgButton", "leftImgButton2", "bottomImgButton", "bottomImgButton2"]
        buttons = []
        for name in bttn_names:
            buttonElem = driver.execute_script(f"return wgtrFind('{name}')")
            if buttonElem == None:
                raise ValueError("Failed to retrieve button element")
                sys.exit(1)
            buttons.append(buttonElem)
        
        # Test 1
        reporter.add_test(1, "Hover behavior test")
        check1_1 = "pointimage change"
        check1_2 = "tristate border change"
        
        # Test 2
        reporter.add_test(2, "Click behavior test")
        check2_1 = "click event updates label"
        check2_2 = "holdimage change"
        
        # Initialize checks
        reporter.record_check(1,check1_1,True)
        reporter.record_check(1,check1_2,True)
        reporter.record_check(2,check2_1,True)
        reporter.record_check(2,check2_2,True)
        
        for button in buttons:
            disabled = not driver.execute_script("return wgtrGetProperty(arguments[0], arguments[1])", button, "enabled")
            type = driver.execute_script("return wgtrGetProperty(arguments[0],arguments[1])",button,"type")
            img = driver.execute_script("return arguments[0].cursrc",button)

            ActionChains(driver).move_to_element(button).perform()

            if not disabled:
                # Test 1 = Hover Behavior
                #   1_1 Pointimage change
                if type != "text":
                    new_img = driver.execute_script("return arguments[0].cursrc",button)
                    if img == new_img:
                        reporter.record_check(1,check1_1,False)

                #   1_2 Tristate change
                tristate = driver.execute_script("return arguments[0].tristate",button)
                if type != "image" and tristate == 1:
                    if driver.execute_script("return arguments[0].mode",button) != 1:
                        reporter.record_check(1,check1_2,False)

            
            prev_label = get_label_info(driver)

            # Test 2 = Click behavior
            ActionChains(driver).click_and_hold(button).perform()
            ActionChains(driver).release(button).perform()
            
            #   2_1 Hover image change
            if not disabled and type != "text":
                holdimage = driver.execute_script("return arguments[0].cursrc",button)
                if holdimage == img:
                    reporter.record_check(2,check2_2,False)
                    
            #   2_2 Verify click result
            has_label_changed =  prev_label != get_label_info(driver)
            if (not disabled and not has_label_changed) or (disabled and has_label_changed):
                reporter.record_check(2,check2_1,False)

    finally:
        # Cleanup
        result = reporter.print_report()
        time.sleep(5)
        driver.quit()
        sys.exit(0) if result else sys.exit(1)


if __name__ == "__main__":
    run_test()