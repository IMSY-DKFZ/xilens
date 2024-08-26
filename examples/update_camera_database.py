###############################################################################################################
#  Author: Intelligent Medical Systems                                                                        #
#  License: see LICENSE.md file                                                                               #
###############################################################################################################
#  This script is capable of scraping the main XIMEA products page                                            #
#  and populates a JSON file with information on camera models and families.                                  #
#                                                                                                             #
#  Usage:                                                                                                     #
#                                                                                                             #
#  To use this script, the following dependencies need to be installed:                                       #
#  `pip install rich typer beautifulsoup4`                                                                    #
#                                                                                                             #
#  To create the database:                                                                                    #
#  `python update_camera_database.py scrap-page`                                                              #
#                                                                                                             #
#  After success, a JSON file is created in the current working directory.                                    #
###############################################################################################################

import json
import threading
import urllib.parse
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import Annotated

import requests
import typer
from bs4 import BeautifulSoup, ResultSet
from rich.progress import Progress, BarColumn, TimeRemainingColumn

app = typer.Typer(no_args_is_help=True, rich_markup_mode="rich")
lock = threading.Lock()


def fetch_main_categories(link_url) -> list:
    """Given a URL, it searches for divs that identify collections of cameras in one single page

    Args:
        link_url: link to product page

    Returns: list of pages each containing list of cameras

    """
    response = requests.get(link_url)
    response.raise_for_status()
    soup = BeautifulSoup(response.content, 'html.parser')
    main_categories = soup.find_all('div', class_='subcategories_list_item_wrapper')
    links = [link['href'] for div in main_categories for link in div.find_all('a', href=True)]
    return links


def fetch_subcategories(link_url: str, subcategory_url: str) -> list:
    """ Given a URL, it searches for divs containing links to a camera page where the camera model anc chroma type can be
    extracted.

    Args:
        link_url: base URL of product page
        subcategory_url: subpage from link_url where the list of cameras can be found

    Returns:

    """
    subcategory_url = urllib.parse.urljoin(link_url, subcategory_url)
    response = requests.get(subcategory_url)
    response.raise_for_status()
    soup = BeautifulSoup(response.content, 'html.parser')
    browse_blocks = soup.find_all('div', class_='browse-page-block')
    links = [link['href'] for div in browse_blocks for link in div.find_all('a', href=True)]
    return links


def fetch_part_numbers(link_url: str) -> dict:
    """ Scraps base url page for sections linking to a list of cameras. Within each subpage, a new search is launched to
    each for each camera page where the camera model, camera family and chroma type are searched for.

    Args:
        link_url: URL page to scrape

    Returns: Dictionary containing the scraped camera models and families

    """
    # Fetch links to all main categories
    main_category_links = fetch_main_categories(link_url)

    part_numbers_data = {}

    # Use ThreadPoolExecutor to process main categories in parallel
    with Progress(
            "[progress.description]{task.description}",
            BarColumn(),
            "[progress.percentage]{task.percentage:>3.0f}%",
            TimeRemainingColumn(),
    ) as progress:
        task = progress.add_task("Scraping subcategories...", total=len(main_category_links))

        with ThreadPoolExecutor(max_workers=10) as executor:
            future_to_subcategory_links = {
                executor.submit(fetch_subcategories, link_url, link): link for link in main_category_links}

            for future in as_completed(future_to_subcategory_links):
                try:
                    subcategory_links = future.result()
                    # Process each subcategory link to get product data
                    with ThreadPoolExecutor(max_workers=10) as inner_executor:
                        future_to_product = {
                            inner_executor.submit(scrape_linked_page, urllib.parse.urljoin(link_url, product_link),
                                                  link_url, part_numbers_data, progress): product_link
                            for product_link in subcategory_links
                        }
                        for future_product in as_completed(future_to_product):
                            try:
                                future_product.result()
                            except Exception as e:
                                print(f"Error occurred while fetching product data: {e}")
                except Exception as e:
                    print(f"Error occurred while fetching subcategories: {e}")
                progress.advance(task)

    return part_numbers_data


def extract_part_numbers_and_chroma(product_divs: ResultSet, soup: BeautifulSoup, part_numbers_data: dict, progress: Progress) -> None:
    """Extracts part numbers and chroma type from a list of divs and updates part numbers data based on the
    extracted part numbers.

    Args:
        product_divs: Set of divs from which the part numbers will be extracted
        soup: BeautifulSoup instance
        part_numbers_data: Dictionary where part numbers are to be stored
        progress: rich progress instance used for logging

    Returns: None

    """
    temp_part_numbers_data = {}

    for div in product_divs:
        part_number = None
        chroma = None

        rows = div.find_all("tr")

        for row in rows:
            caption_span = row.find("span", class_="param_caption")

            # Extract part number
            if caption_span and "Part Number" in caption_span.get_text():
                part_number_td = row.find_all("td")[1]  # Get the following <td> element
                if part_number_td:
                    part_number = part_number_td.get_text().strip()

            # Extract chroma information
            elif caption_span and "Chroma" in caption_span.get_text():
                chroma_td = row.find_all("td")[1]  # Get the following <td> element
                if chroma_td:
                    chroma = chroma_td.get_text().strip()

        # Extract camera family from the second <a class="pathway">
        camera_family = None
        pathways = soup.find_all("a", class_="pathway")
        if len(pathways) >= 2:
            camera_family = pathways[1].get_text().split()[-1]
        if camera_family not in ["xiC", "xiQ", "xiSpec", "xiX", "xiB", "xiB-64", "xiRAY"]:
            progress.log(f"Skipping camera family 'sensors' for camera: {part_number}")
            continue
        if part_number and chroma:
            if "color" in chroma.lower():
                camera_type = "rgb"
            elif  "monochrome" in chroma.lower() or "nir" in chroma.lower():
                camera_type = "gray"
            else:
                progress.log(f"Skipping unknown chroma {chroma} for camera: {part_number}")
                continue
            temp_part_numbers_data[part_number] = {
                "cameraType": camera_type,
                "cameraFamily": camera_family,
                "mosaicWidth": 0,
                "mosaicHeight": 0
            }

    # Update the shared dictionary in a thread-safe manner
    with lock:
        part_numbers_data.update(temp_part_numbers_data)


def scrape_linked_page(link_url: str, base_url: str, part_numbers_data: dict, progress: Progress) -> None:
    """ Scraps page and locates "product-type" divs

    Args:
        link_url: url of page
        base_url: base url of website
        part_numbers_data: dictionary where part numbers are to be stored
        progress: rich progress instance used for logging

    Returns: Nothing

    """
    link_url = urllib.parse.urljoin(base_url, link_url)
    response = requests.get(link_url)
    response.raise_for_status()  # Ensure notice bad responses
    soup = BeautifulSoup(response.content, "html.parser")
    product_divs = soup.find_all("div", class_="product-type")
    extract_part_numbers_and_chroma(product_divs, soup, part_numbers_data, progress)


def sort_by_family_and_part_number(part_numbers_data: dict) -> dict:
    """Sorts content by camera family and within each block of camera families, entries are sorted alphabetically

    Args:
        part_numbers_data: data to be sorted

    Returns: Sorted dictionary

    """
    sorted_data = sorted(part_numbers_data.items(), key=lambda x: (x[1]['cameraFamily'], x[0]))
    sorted_dict = {k: v for k, v in sorted_data}
    return sorted_dict


@app.command(no_args_is_help=True, epilog="Made with :heart: by Leonardo Ayala",
             help="Checks if JSON file contains unique camera models")
def check_json_file(file_path: Annotated[str, typer.Argument(..., help="Path to .json file")]) -> None:
    with open(file_path, "r") as json_file:
        content = json.loads(json_file.read())
    assert len(content.keys()) == len(set(content.keys()))
    print(f"{len(set(content.keys()))} unique camera models found in {file_path}")
    print(f"{len(content.keys())} camera models entries found in {file_path}")


@app.command(no_args_is_help=True, epilog="Made with :heart: by Leonardo Ayala",
             help="Sorts entries in JSON file according to camera family and within each block of camera families, "
                  "entries are sorted alphabetically")
def sort_json_file(file_path: Annotated[str, typer.Argument(..., help="Path to .json file")]) -> None:
    with open(file_path, "r") as json_file:
        content = json.loads(json_file.read())
    sorted_content = sort_by_family_and_part_number(content)
    with open("json_file_sorted.json", "w") as json_file:
        json.dump(sorted_content, json_file, indent=2)


@app.command(epilog="Made with :heart: by Leonardo Ayala",
             help="Scraps main product page to generate a JSON file populated with all camera models")
def scrap_page(link_url: Annotated[str, typer.Option(..., help="Link to page containing overview of camera products,"
                                                          "For example: https://www.ximea.com/en/products")] = "https://www.ximea.com/en/products"):
    part_numbers_data_retrieved = fetch_part_numbers(link_url)
    part_numbers_data_retrieved = sort_by_family_and_part_number(part_numbers_data_retrieved)
    assert len(part_numbers_data_retrieved.keys()) == len(set(part_numbers_data_retrieved.keys()))
    with open("XiLensCameraProperties.json", "w") as json_file:
        json.dump(part_numbers_data_retrieved, json_file, indent=2)

    for part_number_retrieved, data in part_numbers_data_retrieved.items():
        print(f"Part Number: {part_number_retrieved}, Data: {data}")


if __name__ == "__main__":
    app()
