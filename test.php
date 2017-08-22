<?php

$ht = new Linger\Hashtable(6553500);
var_dump($ht);
$ht->set("hello", "world");
var_dump($ht->get("hello"));

$n = 10000;

$start = microtime(true);

for($i = 0; $i < $n; $i++) {
	$ht->set("hello".$i, "world".$i);
}

for($i = 0; $i < $n; $i++) {
	$tmp = $ht->get("hello".$i);
}

echo microtime(true) - $start,PHP_EOL;

var_dump($ht->getSize());
var_dump($ht->getCount());

$start = microtime(true);

for ($i = 0; $i < $n; $i++) {
	$arr["hello".$i] = "world".$i;
}

for ($i = 0; $i < $n; $i++) {
	$tmp = $arr["hello".$i];
}

echo microtime(true) - $start,PHP_EOL;

for ($i = 0; $i < $n; $i++) {
    $res = $ht->del("hello".$i);
    if (!$res) {
        echo "fail:"."hello".$i,PHP_EOL;
    }
}

var_dump($ht->getCount());

