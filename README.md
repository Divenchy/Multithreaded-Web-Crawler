# Multithreaded Web Crawler

![Screenshot 2025-03-13 002418](https://github.com/user-attachments/assets/24139336-7001-4c29-be7e-19bf14512096)

Primarily written in C. Extending my working simple web crawler with multithreading capability in order for large crawls (1M links). Uses a txt file that contains one url per line,
and has HTML parsing capability. Program status is printed to the console every 2 secs detailing current active threads, num of urls extracted from file, number of
unique connections, etc.

## Current Issues
- Enhance memory safety and performance
- Correct behavior when utilized with 3000+ threads (synchronization issue)
- Implement crawl result statistics (remove placeholder printout)
