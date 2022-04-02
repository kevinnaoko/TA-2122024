function initMap() {
    var coords = { lat: {{ lat }}, lng: {{ lng }} };
    var map = new google.maps.Map(document.getElementById('map'),
        {
            center: coords,
            disableDefaultUI: true,
    zoom: 16
    });

    var marker = new google.maps.Marker({
        position: coords,
        map: map
    });

}