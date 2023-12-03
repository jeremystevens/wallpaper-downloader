#!/bin/python
# Copyright 2023 Jeremy Stevens

# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
#  arising from the use of this software.

#  Permission is granted to anyone to use this software for any purpose,
#  including commercial applications, and to alter it and redistribute it
#  freely, subject to the following restrictions:

#  1. The origin of this software must not be misrepresented; you must not
#     claim that you wrote the original software. If you use this software
#     in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#  2. Altered source versions must be plainly marked as such, and must not be
#     misrepresented as being the original software.
#  3. This notice may not be removed or altered from any source distribution.
#
# Leave a comment or email <jeremiahstevens@gmail.com> for feature request or future project ideas

"""
Wallpaper Manager
------------------

A script that provides two main functionalities:
1. Download random wallpapers from Unsplash.
2. Identify and remove duplicate wallpapers from a specified directory.

User can set the `DIRECTORY_PATH` variable to specify the target directory.
"""

__version__ = '0.0.1'

import os
import cv2
from PIL import Image
from PIL import UnidentifiedImageError
from simple_term_menu import TerminalMenu
import requests
from tqdm import tqdm
from time import sleep
import datetime

 ### EDIT THIS LINE ### 
DIRECTORY_PATH = "dir/to/save"
WALLPAPER_DIR = DIRECTORY_PATH  # Make sure it's consistent with the above path

def dhash(image, hash_size=8):
	
     """
    Compute the difference hash of an image.
    
    Parameters:
    - image (PIL.Image): The input image.
    - hash_size (int): The size of the hash.
    
    Returns:
    - int: The hash value.
    """
    resized = image.resize((hash_size + 1, hash_size))
    diff = []
    for row in range(hash_size):
        for col in range(hash_size):
            diff.append(resized.getpixel((col, row)) > resized.getpixel((col + 1, row)))
    return sum([2 ** i for i, v in enumerate(diff) if v])

def get_image_files(folder_path):
    extensions = ['jpg', 'jpeg', 'png', 'bmp']
    return [os.path.join(folder_path, f) for f in os.listdir(folder_path) if f.split('.')[-1].lower() in extensions]

def find_duplicates(directory):
    image_files = get_image_files(directory)
    hashes = {}
    failed_images = []  # List to keep track of images that failed to process

    for image_path in image_files:
        try:
            img = Image.open(image_path).convert('L')  # Convert image to grayscale
            h = dhash(img)
            if h in hashes:
                hashes[h].append(image_path)
            else:
                hashes[h] = [image_path]
        except (IOError, UnidentifiedImageError):  
            print(f"Unable to process {image_path}.")
            failed_images.append(image_path)  # Append the failed image to the list
            
    duplicates = [imgs for h, imgs in hashes.items() if len(imgs) > 1]

    return duplicates, failed_images  # Return both duplicates and failed images


def delete_files(file_list):
    choice = input(f"Do you want to delete {len(file_list)} files? (yes/no): ").strip().lower()
    
    if choice == 'yes':
        for file_path in file_list:
            try:
                os.remove(file_path)
                print(f"Deleted: {file_path}")
            except Exception as e:
                print(f"Error deleting {file_path}: {e}")
    elif choice == 'no':
        print("Files were not deleted.")
    else:
        print("Invalid choice. Files were not deleted.")

def download_wallpapers():
    """
    Downloads wallpapers based on user's input for count and sleep duration.
    """
    if not os.path.exists(WALLPAPER_DIR):
        os.makedirs(WALLPAPER_DIR)

    max_wallpapers = int(input("Enter the maximum number of wallpapers to download (default: 100): ") or 100)
    sleep_duration = int(input("Enter the sleep duration in seconds between downloading wallpapers (default: 300): ") or 300)

    wallpaper_count = 0

    for i in range(max_wallpapers):
        if download_wallpaper():  # Check if download was successful
            wallpaper_count += 1
            print(f"Downloaded {wallpaper_count} out of {max_wallpapers} wallpapers.")
            
            if i < max_wallpapers - 1:  # No need to sleep after the last wallpaper is downloaded
                sleep(sleep_duration)
    
    print(f"Maximum number of wallpapers ({max_wallpapers}) downloaded. Exiting...")

def download_wallpaper():
    """
    Downloads a random wallpaper from Unsplash and saves it with a unique filename.
    
    Returns:
    - bool: True if download is successful. Currently, it always returns True.
    """
    # Generate a unique filename based on the current timestamp
    timestamp = datetime.datetime.now().strftime('%Y%m%d%H%M%S')
    unique_filename = os.path.join(WALLPAPER_DIR, f"wallpaper_{timestamp}.jpg")

    # Get redirect URL for the image
    response = requests.get('https://source.unsplash.com/random/1920x1080', stream=True, allow_redirects=True)
    image_url = response.url

    # Download the image and show progress bar
    response = requests.get(image_url, stream=True)
    total_size = int(response.headers.get('content-length', 0))
    
    with open(unique_filename, 'wb') as file, tqdm(
        desc=unique_filename,
        total=total_size,
        unit='B',
        unit_scale=True,
        unit_divisor=1024,
    ) as bar:
        for data in response.iter_content(1024):
            bar.update(len(data))
            file.write(data)
    return True

#### Main Menu #####
def main_menu():
    """
    Displays the main menu for the script using simple_term_menu.
    Provides the user with options to download wallpapers or remove duplicates.
    """
    options = ["Download Wallpapers", "Remove Duplicates"]
    terminal_menu = TerminalMenu(options, title="Select an action:")
    menu_entry_index = terminal_menu.show()
    
    if menu_entry_index == 0:
        download_wallpapers()
    elif menu_entry_index == 1:
        dupes, failed = find_duplicates(DIRECTORY_PATH)  # Use the global constant

        if not dupes:  # If no duplicates found
            print("No duplicates found.")
        else:
            for group in dupes:
                print("Duplicate group:")
                for image_path in group:
                    print(image_path)
            delete_files([item for sublist in dupes for item in sublist[1:]])

        if failed:
            print("\nThe following images were unable to process:")
            for image_path in failed:
                print(image_path)
            print("\nThese images might be corrupted or broken.")
            delete_files(failed)
                    
if __name__ == "__main__":

	main_menu()
