<?php

session_start();
header("Content-Type: application/x-javascript");

unset($_SESSION["user"]);
session_unset();
session_destroy();

?>