"# basic-webserver" 
Simple web server using C

1. Compile 
gcc -Wall -Wextra -g -o simple simple.c

2. Run 
./simple


You can add web pages, css and images in to htdocs and access them through  http://localhost:8080/path/to/your/resouce 
You can access json in ./api folder if the request's Accept heade is applicaton/json (ideally this should be determined based on route eg localhost:8080/api/v1/colors but I kept this simple)
This server does not support any method other than GET

Features 
Error handling - 500, 405, 404, 400 Error response
Supports Json, html, css, jpg, webp 
File system based routing  


Optimized server - simple_optimized

Usage of Pthread to enhance server perfomance 