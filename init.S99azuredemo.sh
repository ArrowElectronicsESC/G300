echo 'Starting g300 service...' >&2
/data/rundemo.sh &
usockc /var/gpio_ctrl WLAN_LED_RED
echo 'g300 Service started' >&2