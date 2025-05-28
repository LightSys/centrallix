# David Hopkins May 2025
# NOTE: USE ChromeDriverManager. Pip install it.
# Updated: May 2025 (Expanding more nodes)

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.common.exceptions import TimeoutException, ElementClickInterceptedException, NoSuchFrameException
from datetime import datetime
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager

def create_driver(test_url) -> webdriver.Chrome:
    """Create and return a configured Chrome WebDriver."""
    service = Service(ChromeDriverManager().install())
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_options.add_argument('--ignore-certificate-errors')
    chrome_options.add_argument('--disable-gpu')
    chrome_options.add_argument('--no-sandbox')
    chrome_options.add_argument('--disable-features=TensorFlowLite')
    chrome_options.add_argument('--disable-dev-shm-usage')
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
    """Run the button functionality test slowly for visibility."""
    driver = None
    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/scrollpane_test.app"
        driver = create_driver(test_url)

        # Wait for the page to load and framework to initialize
        WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.TAG_NAME, "body"))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Body element found.")
        time.sleep(2)

        WebDriverWait(driver, 20).until(
            lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded")
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Framework initialized.")
        time.sleep(5)

        # Locate the treeview node for "/"
        treeview_node = WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((
                By.XPATH,
                "//div[starts-with(@class, 'tv') and contains(., '/')]//img[contains(@src, 'sys/images/ico02b.gif')]"
            ))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Treeview root node located.")

        # Ensure the treeview node is visible by adjusting the clip style
        treeview_container = treeview_node.find_element(By.XPATH, "..")
        driver.execute_script("arguments[0].style.clip = 'auto';", treeview_container)
        time.sleep(1)

        # Locate the scrollpane area dynamically
        scrollpane_area = treeview_node.find_element(By.XPATH, "./ancestor::div[contains(@style, 'clip: rect') and contains(@style, 'z-index: 14')]")
        content_height = driver.execute_script("return arguments[0].scrollHeight;", scrollpane_area)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Scrollable content height: {content_height}")
        time.sleep(1)

        # Scroll the element into the viewport
        driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", treeview_node)
        time.sleep(1)

        actions = ActionChains(driver) # Initialize ActionChains here

        # Click the root node to expand it
        treeview_button = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable((
                By.XPATH,
                "//div[starts-with(@class, 'tv') and contains(., '/')]//img[contains(@src, 'sys/images/ico02b.gif')]"
            ))
        )
        actions.move_to_element(treeview_button).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Treeview root node clicked via ActionChains.")
        WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located((
                By.XPATH,
                "//div[starts-with(@class, 'tv') and contains(., '/')]/following-sibling::div[starts-with(@class, 'tv')]"
            ))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Treeview root expanded.")
        time.sleep(2)

        # Click the "apps" node to expand it
        apps_node = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable((
                By.XPATH,
                "//div[starts-with(@class, 'tv') and contains(., 'apps')]//img[contains(@src, 'sys/images/ico02b.gif')]"
            ))
        )
        driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", apps_node)
        time.sleep(1)
        actions.move_to_element(apps_node).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Apps node clicked via ActionChains.")
        WebDriverWait(driver, 10).until(
            EC.presence_of_all_elements_located((
                By.XPATH,
                "//div[starts-with(@class, 'tv') and contains(., 'apps')]/following-sibling::div[starts-with(@class, 'tv')]"
            ))
        )
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Apps node expanded.")
        time.sleep(2)

        # Click the "kardia" node to expand it
        kardia_node = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable((
                By.XPATH,
                "//div[starts-with(@class, 'tv') and contains(., 'kardia')]//img[contains(@src, 'sys/images/ico02b.gif')]"
            ))
        )
        driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", kardia_node)
        time.sleep(1)
        actions.move_to_element(kardia_node).click().perform()
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Kardia node clicked via ActionChains.")
        time.sleep(2)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Kardia node expanded (or checked).")

        # --- NEW: Click the "sys" node to expand it ---
        try:
            sys_node = WebDriverWait(driver, 10).until(
                EC.element_to_be_clickable((
                    By.XPATH,
                    "//div[starts-with(@class, 'tv') and contains(., 'sys')]//img[contains(@src, 'sys/images/ico02b.gif')]"
                ))
            )
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Sys node located.")
            driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", sys_node)
            time.sleep(1)
            actions.move_to_element(sys_node).click().perform()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Sys node clicked via ActionChains.")
            WebDriverWait(driver, 10).until(
                EC.presence_of_all_elements_located((
                    By.XPATH,
                    "//div[starts-with(@class, 'tv') and contains(., 'sys')]/following-sibling::div[starts-with(@class, 'tv')]"
                ))
            )
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Sys node expanded.")
            time.sleep(2)
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Could not expand 'sys' node: {e}")

        try:
            tests_node = WebDriverWait(driver, 10).until(
                EC.element_to_be_clickable((
                    By.XPATH,
                    "//div[starts-with(@class, 'tv') and contains(., 'tests')]//img[contains(@src, 'sys/images/ico02b.gif')]"
                ))
            )
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Tests node located.")
            driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", tests_node)
            time.sleep(1)
            actions.move_to_element(tests_node).click().perform()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Tests node clicked via ActionChains.")
            WebDriverWait(driver, 10).until(
                EC.presence_of_all_elements_located((
                    By.XPATH,
                    "//div[starts-with(@class, 'tv') and contains(., 'tests')]/following-sibling::div[starts-with(@class, 'tv')]"
                ))
            )
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Tests node expanded.")
            time.sleep(2)
        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Could not expand 'tests' node: {e}")

        # Recompute content height after expansion
        content_height = driver.execute_script("return arguments[0].scrollHeight;", scrollpane_area)
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Updated scrollable content height: {content_height}")


        def get_scroll_positions(pane):
            """Helper to get both top and scrollTop, handling potential errors."""
            top_pos = driver.execute_script("return window.getComputedStyle(arguments[0]).top;", pane)
            scroll_top = driver.execute_script("return arguments[0].scrollTop;", pane)
            try:
                top_float = float(top_pos.replace('px', ''))
            except (ValueError, AttributeError):
                top_float = None # Handle cases where it might not be 'px' or a number
            return top_pos, top_float, scroll_top

        # 1. Click the down arrow (name="d")
        try:
            down_arrow = WebDriverWait(driver, 10).until(
                EC.element_to_be_clickable((By.XPATH, "//img[@name='d']"))
            )
            driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", down_arrow)
            time.sleep(1) # Wait after scroll

            initial_top_str, initial_top_float, initial_scroll_top = get_scroll_positions(scrollpane_area)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Before down click: Top={initial_top_str}, ScrollTop={initial_scroll_top}")

            # Use ActionChains for click
            actions.move_to_element(down_arrow).click().perform()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Down arrow clicked via ActionChains.")
            time.sleep(3) # Wait for potential scroll

            new_top_str, new_top_float, new_scroll_top = get_scroll_positions(scrollpane_area)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - After down click: Top={new_top_str}, ScrollTop={new_scroll_top}")

            # Verify scroll (check both Top and ScrollTop)
            if initial_top_float is not None and new_top_float is not None and new_top_float < initial_top_float:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Down arrow scroll verified (Top changed).")
            elif new_scroll_top > initial_scroll_top:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Down arrow scroll verified (ScrollTop changed).")
            else:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Down arrow scroll FAILED (position didn't change).")

        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Error clicking down arrow: {str(e)}")
            raise

        # 2. Click the up arrow (name="u")
        try:
            up_arrow = WebDriverWait(driver, 10).until(
                EC.element_to_be_clickable((By.XPATH, "//img[@name='u']"))
            )
            driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", up_arrow)
            time.sleep(1)

            initial_top_str, initial_top_float, initial_scroll_top = get_scroll_positions(scrollpane_area)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Before up click: Top={initial_top_str}, ScrollTop={initial_scroll_top}")

            # Use ActionChains for click
            actions.move_to_element(up_arrow).click().perform()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Up arrow clicked via ActionChains.")
            time.sleep(3) # Wait for potential scroll

            new_top_str, new_top_float, new_scroll_top = get_scroll_positions(scrollpane_area)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - After up click: Top={new_top_str}, ScrollTop={new_scroll_top}")

            # Verify scroll
            if initial_top_float is not None and new_top_float is not None and new_top_float > initial_top_float:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Up arrow scroll verified (Top changed).")
            elif new_scroll_top < initial_scroll_top:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Up arrow scroll verified (ScrollTop changed).")
            else:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Up arrow scroll FAILED (position didn't change).")

        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Error clicking up arrow: {str(e)}")
            raise

        # 3. Drag the scrollbar thumb (name="t")
        try:
            thumb = WebDriverWait(driver, 10).until(
                EC.element_to_be_clickable((By.XPATH, "//img[@name='t']"))
            )
            driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", thumb)
            time.sleep(1)

            initial_top_str, initial_top_float, initial_scroll_top = get_scroll_positions(scrollpane_area)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Before drag: Top={initial_top_str}, ScrollTop={initial_scroll_top}")

            drag_offset_y = 100 # Adjust offset as needed
            # Perform drag using ActionChains
            actions.move_to_element(thumb).click_and_hold().move_by_offset(0, drag_offset_y).release().perform()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Scrollbar thumb dragged {drag_offset_y}px downward.")
            time.sleep(3) # Wait for potential scroll

            new_top_str, new_top_float, new_scroll_top = get_scroll_positions(scrollpane_area)
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - After drag: Top={new_top_str}, ScrollTop={new_scroll_top}")

            # Verify scroll
            if initial_top_float is not None and new_top_float is not None and new_top_float < initial_top_float:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Scrollbar drag verified (Top changed).")
            elif new_scroll_top > initial_scroll_top:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Scrollbar drag verified (ScrollTop changed).")
            else:
                print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Scrollbar drag FAILED (position didn't change).")

        except Exception as e:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} -  Error dragging scrollbar thumb: {str(e)}")
            raise


    except FileNotFoundError:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Config.toml is missing.")
    except TimeoutException as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Timeout: {e}")
        if driver:
            with open("page_dump.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            driver.save_screenshot("error_screenshot.png")
            print("Page HTML dumped to page_dump.html and screenshot saved to error_screenshot.png")
    except ElementClickInterceptedException as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Click intercepted: {e}")
        if driver:
            with open("page_dump.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            driver.save_screenshot("error_screenshot.png")
            print("Page HTML dumped to page_dump.html and screenshot saved to error_screenshot.png")
    except NoSuchFrameException as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Iframe not found: {e}")
        if driver:
            with open("page_dump.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            driver.save_screenshot("error_screenshot.png")
            print("Page HTML dumped to page_dump.html and screenshot saved to error_screenshot.png")
    except Exception as e:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Unexpected error: {e}")
        if driver:
            with open("page_dump.html", "w", encoding="utf-8") as f:
                f.write(driver.page_source)
            driver.save_screenshot("error_screenshot.png")
            print("Page HTML dumped to page_dump.html and screenshot saved to error_screenshot.png")
    finally:
        print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Test complete.")
        time.sleep(2)
        if driver:
            driver.quit()
            print(f"{datetime.now().strftime('%H:%M:%S.%f')} - Driver closed.")

if __name__ == "__main__":
    run_test()