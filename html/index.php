<?php
// Extract current count, defaulting to 0
$count = 0;
if (isset($_COOKIE['visit_count'])) {
    $count = (int)$_COOKIE['visit_count'];
}

// Increment the count
$count++;

// Output CGI Headers
echo "Status: 200 OK\r\n";
echo "Content-Type: text/plain\r\n";
echo "Set-Cookie: visit_count=" . $count . "\r\n";
echo "\r\n";

// Output plain text body
echo "// welcome to webserv (" . $count . ")\n";
?>
