""" module allowing web testing """

import time
import asyncio
import toml
import imagehash as ImageHash
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from PIL import Image
from pyppeteer import launch

def create_driver(test_url: str) -> webdriver:
    """ Create a WebDriver """
    service = Service(executable_path="chromedriver.exe")
    chrome_options = webdriver.ChromeOptions()
    chrome_options.add_argument('--lang=en')
    chrome_options.add_argument('--incognito')
    chrome_driver = webdriver.Chrome(service=service, options = chrome_options)
    chrome_driver.set_window_size(1920, 1080)

    # Replace credentials
    chrome_driver.get(test_url)

    return chrome_driver

def compare_images(img1: str, img2: str) -> bool:
    """ Compare two images """
    img1 = Image.open(img1)
    img2 = Image.open(img2)

    hash1 = ImageHash.phash(img1)
    hash2 = ImageHash.phash(img2)

    return hash1 - hash2

async def take_screenshots(time_stamp: str, c_top: int, c_left: int, c_width: int, c_height: int, test_url: str):
    """ Take a screenshot of the page """
    browser = await launch(executablePath='C:/Program Files/Google/Chrome/Application/chrome.exe')
    page = await browser.newPage()

    # Replace credentials
    await page.goto(test_url)

    element = await page.waitForXPath('//input[1]')
    await element.hover()
    container = { 'x': c_left, 'y': c_top, 'width': c_width, 'height': c_height }

    await page.screenshot({'path': f'test_tb_{time_stamp}_hover.png', 'clip': container})

    await element.click()
    await page.screenshot({'path': f'test_tb_{time_stamp}_click.png', 'clip': container})

    await browser.close()

# Configure settings
config = toml.load("config.toml")

user, password, url, port = config["user"], config["pw"], config["url"], config["port"]

proper_url = f"http://{user}:{password}@{url}:{port}/tests/ui/textbutton_test.app"

# Create the WebDriver
driver = create_driver(proper_url)

WebDriverWait(driver, 5).until(
    EC.presence_of_element_located((By.XPATH, "//div[last()]"))
)

button1 = driver.find_element(By.XPATH, "//div[last() - 1]")

# Get the dimensions (position and size) of the widget
dims = driver.execute_script("""
return {
    top: wgtrFind('vbox').clientTop,
    left: wgtrFind('vbox').clientLeft,
    width: wgtrFind('vbox').clientWidth,
    height: wgtrFind('vbox').clientHeight
}
""")
top, left, width, height = dims['top'], dims['left'], dims['width'], dims['height']

# Get timestamp
timestamp = time.strftime("%Y%m%d-%H%M%S")

# Take screenshots
asyncio.get_event_loop().run_until_complete(take_screenshots(timestamp, top, left, width, height, proper_url))

# Close the browser
driver.quit()

# Compare the screenshots
hover_diff = compare_images(f'test_tb_{timestamp}_hover.png', 'ss_textbutton_hover.png')
clicked_diff = compare_images(f'test_tb_{timestamp}_click.png', 'ss_textbutton_click.png')

# Print the results
print("\nDifference (lower is better): ")
print(f"Mouseover Test (0-64): {hover_diff}")
print(f"Click Test (0-64): {clicked_diff}\n")
