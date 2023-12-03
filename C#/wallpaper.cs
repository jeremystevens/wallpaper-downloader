using System;
using System.IO;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

/*
 * Copyright (C) 2023  Jeremy Stevens
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

class Program
{
    static async Task Main(string[] args)
    {
        // Display the banner
        Console.WriteLine(@"
 █░█░█▀█░█░░░█░░░█▀█░█▀█░█▀█░█▀▀░█▀▄░░░█▀▄░█▀█░█░█░█▀█░█░░░█▀█░█▀█░█▀▄░█▀▀░█▀▄
░█▄█░█▀█░█░░░█░░░█▀▀░█▀█░█▀▀░█▀▀░█▀▄░░░█░█░█░█░█▄█░█░█░█░░░█░█░█▀█░█░█░█▀▀░█▀▄
░▀░▀░▀░▀░▀▀▀░▀▀▀░▀░░░▀░▀░▀░░░▀▀▀░▀░▀░░░▀▀░░▀▀▀░▀░▀░▀░▀░▀▀▀░▀▀▀░▀░▀░▀▀░░▀▀▀░▀░▀
Rev 3.5 by Jeremy Stevens
");

        // Get user input for screen resolution
        Console.Write("Enter your screen resolution (e.g., 1920x1080): ");
        string desiredResolution = Console.ReadLine();

        // Get user input for download mode (random or keyword)
        Console.Write("Choose the download mode (random/keyword): ");
        string downloadMode = Console.ReadLine();

        string keyword = null;

        if (downloadMode == "keyword")
        {
            Console.Write("Enter a keyword for wallpapers: ");
            keyword = Console.ReadLine();
        }

        Console.Write("Enter the maximum number of wallpapers to download (default: 100): ");
        if (!int.TryParse(Console.ReadLine(), out int maxWallpapers))
        {
            maxWallpapers = 100;
        }

        Console.Write("Enter the sleep duration in seconds between downloading wallpapers (default: 300): ");
        if (!int.TryParse(Console.ReadLine(), out int sleepDuration))
        {
            sleepDuration = 300;
        }

        // Get the current logged-in user
        string user = Environment.UserName;

        // Define the directory where wallpapers will be stored
        string wallpaperDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Pictures");

        // Path to the download history file
        string downloadHistoryFile = Path.Combine(wallpaperDir, "download_history.txt");

        // Set the initial value of the downloaded wallpaper counter to 0
        int downloadedWallpapers = 0;

        // Create the directory if it doesn't exist
        if (!Directory.Exists(wallpaperDir))
        {
            Directory.CreateDirectory(wallpaperDir);
        }

        // Create the download history file if it doesn't exist
        if (!File.Exists(downloadHistoryFile))
        {
            File.Create(downloadHistoryFile).Close();
        }

        while (downloadedWallpapers < maxWallpapers)
        {
            // Download the wallpaper based on the chosen mode (random or keyword)
            string imageUrl = downloadMode == "random"
                ? await GetRandomImage(desiredResolution)
                : await GetKeywordImage(desiredResolution, keyword);

            // Calculate the MD5 hash of the image
            string imageHash = CalculateMd5Hash(await DownloadImage(imageUrl));

            // Check if the image hash is already in the download history
            if (!IsImageAlreadyDownloaded(downloadHistoryFile, imageHash))
            {
                // Save the image to the wallpaper directory
                string uniqueFilename = $"wallpaper_{DateTime.Now:yyyyMMddHHmmss}.jpg";
                string imagePath = Path.Combine(wallpaperDir, uniqueFilename);
                await SaveImage(imageUrl, imagePath);

                // Append the image hash to the download history file
                File.AppendAllText(downloadHistoryFile, imageHash + Environment.NewLine);

                // Increment the downloaded wallpaper counter
                downloadedWallpapers++;

                Console.WriteLine($"Downloaded wallpaper {downloadedWallpapers} of {maxWallpapers}");
            }

            // Sleep for the specified duration before downloading the next wallpaper
            Thread.Sleep(sleepDuration * 1000);
        }

        Console.WriteLine($"Maximum number of wallpapers ({maxWallpapers}) downloaded. Exiting...");
    }

    static async Task<string> GetRandomImage(string resolution)
    {
        string imageUrl = $"https://source.unsplash.com/random/{resolution}";
        return await GetFinalImageUrl(imageUrl);
    }

    static async Task<string> GetKeywordImage(string resolution, string keyword)
    {
        string imageUrl = $"https://source.unsplash.com/featured/{resolution}/?{keyword}";
        return await GetFinalImageUrl(imageUrl);
    }

    static async Task<string> GetFinalImageUrl(string imageUrl)
    {
        using (HttpClient httpClient = new HttpClient())
        {
            HttpResponseMessage response = await httpClient.GetAsync(imageUrl, HttpCompletionOption.ResponseHeadersRead);
            return response.RequestMessage.RequestUri.ToString();
        }
    }

    static async Task<byte[]> DownloadImage(string imageUrl)
    {
        using (HttpClient httpClient = new HttpClient())
        {
            return await httpClient.GetByteArrayAsync(imageUrl);
        }
    }

    static string CalculateMd5Hash(byte[] data)
    {
        using (MD5 md5 = MD5.Create())
        {
            byte[] hash = md5.ComputeHash(data);
            StringBuilder hashBuilder = new StringBuilder();
            for (int i = 0; i < hash.Length; i++)
            {
                hashBuilder.Append(hash[i].ToString("x2"));
            }
            return hashBuilder.ToString();
        }
    }

    static bool IsImageAlreadyDownloaded(string downloadHistoryFile, string imageHash)
    {
        if (!File.Exists(downloadHistoryFile))
        {
            return false;
        }

        string[] lines = File.ReadAllLines(downloadHistoryFile);
        return Array.Exists(lines, line => line == imageHash);
    }

    static async Task SaveImage(string imageUrl, string imagePath)
    {
        using (HttpClient httpClient = new HttpClient())
        {
            byte[] imageData = await httpClient.GetByteArrayAsync(imageUrl);
            File.WriteAllBytes(imagePath, imageData);
        }
    }
}
