<?php

$ht = new linger\Hashtable();

$n = 10000;

$start = microtime(true);
for ($i = 0; $i < $n; $i++) {
    $ht->set("hello{$i}", "world{$i}");
}

for ($i = 0; $i < $n; $i++) {
    $ht->get("hello{$i}");
}

echo "HashTable:" . (microtime(true) - $start), PHP_EOL;

$arr = [];

$start = microtime(true);
for ($i = 0; $i < $n; $i++) {
   $arr["hello{$i}"] = "world{$i}"; 
}

for ($i = 0; $i < $n; $i++) {
    $tmp = $arr["hello{$i}"];
}
echo "Array:" . (microtime(true) - $start), PHP_EOL;
