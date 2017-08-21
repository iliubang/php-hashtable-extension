<?php

$ht = new Linger\Hashtable(65535);
var_dump($ht);
$ht->set("hello", "world");
var_dump($ht->get("hello"));

$n = 1000;

$start = microtime(true);

for($i = 0; $i < $n; $i++) {
	$ht->set("hello".$i, "world".$i);
}

for($i = 0; $i < $n; $i++) {
	var_dump($ht->get("hello".$i));
}

echo microtime(true) - $start,PHP_EOL;

var_dump($ht->getSize());
