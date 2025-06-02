#David Hopkins June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.common.exceptions import TimeoutException, ElementClickInterceptedException
from datetime import datetime
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager

def create_driver(test_url) -> webdriver.Chrome:
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_options.add_argument('--ignore-certificate-errors')
    driver = webdriver.Chrome(service=service, options=chrome_options)
    driver.set_window_size(1920, 1080)
    driver.get(test_url)
    WebDriverWait(driver, 10).until(
        lambda d: d.execute_script("return document.readyState") == "complete"
    )
    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Page loaded.")
    time.sleep(2)
    return driver

def run_test():
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Config.toml is missing.")
        return

    test_url = config["url"] + "/tests/ui/childwindow_test.app"
    driver = create_driver(test_url)

    try:
        WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.TAG_NAME, "body"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Body element found.")
        time.sleep(1)

        WebDriverWait(driver, 20).until(
            lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded")
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Framework initialized.")

        WebDriverWait(driver, 20).until(
            lambda d: d.execute_script("return typeof wn_topwin !== 'undefined' && wn_topwin !== null")
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Window initialized.")
        time.sleep(1)

        # Find the title bar
        try:
            titlebar = WebDriverWait(driver, 10).until(
                EC.element_to_be_clickable((By.CSS_SELECTOR, "div.wntitlebar"))
            )
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Titlebar (.wntitlebar) found.")
            time.sleep(1)

            # Log initial position
            initial_position = driver.execute_script("""
                var window = document.querySelector('div.wnbase');
                return window ? { left: window.style.left, top: window.style.top } : null;
            """)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Initial window position: {initial_position}")

            # Attempt ActionChains drag with overlay workaround
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Attempting ActionChains drag.")
            try:
                # Temporarily disable pointer-events on overlay
                driver.execute_script("""
                    var overlay = document.querySelector('div[style*="black_trans_50.png"]');
                    if (overlay) overlay.style.pointerEvents = 'none';
                """)
                actions = ActionChains(driver)
                actions.move_to_element_with_offset(titlebar, 10, 10).click_and_hold().perform()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Clicked and held titlebar.")
                time.sleep(0.5)
                actions.move_by_offset(500, 0).perform()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Dragged window right by 500px.")
                time.sleep(0.5)
                actions.release().perform()
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Released titlebar.")
                # Restore pointer-events
                driver.execute_script("""
                    var overlay = document.querySelector('div[style*="black_trans_50.png"]');
                    if (overlay) overlay.style.pointerEvents = 'auto';
                """)
                # Log final position
                final_position = driver.execute_script("""
                    var window = document.querySelector('div.wnbase');
                    return window ? { left: window.style.left, top: window.style.top } : null;
                """)
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Final window position (ActionChains): {final_position}")
                if initial_position == final_position:
                    print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Warning: Window position did not change (ActionChains).")
            except (TimeoutException, ElementClickInterceptedException) as e:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error with ActionChains drag: {e}")

            # JavaScript-based drag (fallback, works with overlay)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Attempting JavaScript-based drag.")
            driver.execute_script("""
                var titlebar = document.querySelector('div.wntitlebar');
                var mousedown = new MouseEvent('mousedown', { bubbles: true, clientX: 100, clientY: 100 });
                var mousemove = new MouseEvent('mousemove', { bubbles: true, clientX: 600, clientY: 100 });
                var mouseup = new MouseEvent('mouseup', { bubbles: true, clientX: 600, clientY: 100 });
                titlebar.dispatchEvent(mousedown);
                setTimeout(function() {
                    titlebar.dispatchEvent(mousemove);
                    setTimeout(function() {
                        titlebar.dispatchEvent(mouseup);
                    }, 500);
                }, 500);
            """)
            time.sleep(2)
            js_final_position = driver.execute_script("""
                var window = document.querySelector('div.wnbase');
                return window ? { left: window.style.left, top: window.style.top } : null;
            """)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Final window position (JS drag): {js_final_position}")

            #Now, try to double click the titlebar to minimize the window, then double clicking it again to restore it.
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Attempting to double click titlebar to minimize.")
            actions = ActionChains(driver)
            actions.move_to_element(titlebar).double_click().perform()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Titlebar double-clicked to minimize.")
            time.sleep(1)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Attempting to double click titlebar to restore.")
            actions.move_to_element(titlebar).double_click().perform()
            time.sleep(3)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Titlebar double-clicked to restore.")

            #Now, try to click the x window close button. (td align right, class="wntitlebar" )
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Attempting to click close button.")
            close_button = WebDriverWait(driver, 10).until(
                EC.element_to_be_clickable((By.CSS_SELECTOR, "td[align='right']"))
            )
            time.sleep(3)  # Pause to ensure button is ready
            close_button.click()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Close button clicked.")

        except (TimeoutException, ElementClickInterceptedException) as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Error finding titlebar: {e}")

        
    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete, keeping browser open for observation.")
        time.sleep(5)
        driver.quit()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()