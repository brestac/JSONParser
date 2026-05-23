#pragma once

#include <LittleFS.h>
#include <math.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
//  Helpers internes
// ---------------------------------------------------------------------------

static void _geojson_write_polygon(File& f, double cx, double cy,
                                   int rings, int points_per_ring) {
    f.print("{\"type\":\"Polygon\",\"coordinates\":[");

    for (int r = 0; r < rings; r++) {
        if (r > 0) f.print(",");

        double radius = 0.01 + (rand() % 100) * 0.001;

        f.print("[");
        for (int i = 0; i <= points_per_ring; i++) {
            if (i > 0) f.print(",");

            double angle = (2.0 * M_PI * i) / points_per_ring;
            double lon   = cx + radius * cos(angle);
            double lat   = cy + radius * sin(angle);

            if (lon >  180.0) lon =  180.0;
            if (lon < -180.0) lon = -180.0;
            if (lat >   90.0) lat =   90.0;
            if (lat <  -90.0) lat =  -90.0;

            f.print("[");
            f.print(lon, 6);
            f.print(",");
            f.print(lat, 6);
            f.print("]");
        }
        f.print("]");
    }

    f.print("]}");
}

// ---------------------------------------------------------------------------
//  generate_geojson
//
//  Génère un fichier GeoJSON d'environ target_kb kilo-octets dans LittleFS.
//
//  Paramètres :
//    path            – chemin dans LittleFS, ex: "/test.geojson"
//    target_kb       – taille cible en KB
//    rings           – nombre de rings par polygone (défaut 1)
//    points_per_ring – points par ring          (défaut 8)
//
//  Retourne la taille réelle écrite en octets, ou -1 en cas d'erreur.
// ---------------------------------------------------------------------------

long generate_geojson(const char* path, size_t target_kb,
                      int rings = 1, int points_per_ring = 8) {

    // Supprimer le fichier existant éventuel
    if (LittleFS.exists(path)) {
        LittleFS.remove(path);
    }

    File f = LittleFS.open(path, "w");
    if (!f) {
        Serial.printf("[GeoJSON] Erreur: impossible d'ouvrir '%s'\n", path);
        return -1;
    }

    randomSeed(micros());

    const size_t target_bytes = target_kb * 1024UL;

    f.print("{\"type\":\"FeatureCollection\",\"features\":[");

    size_t written    = 0;
    int    feat_count = 0;

    while (true) {
        // Coordonnées aléatoires
        double cx = -180.0 + (random(36000)) * 0.01;
        double cy =  -90.0 + (random(18000)) * 0.01;

        // Estimation de la taille de la prochaine feature
        size_t est = 120 + (size_t)(rings * (points_per_ring + 1)) * 22;

        if (written + est > target_bytes * 95 / 100)
            break;

        if (feat_count > 0) f.print(",");

        char name[32];
        snprintf(name, sizeof(name), "feature_%d", feat_count);

        f.print("{\"type\":\"Feature\","
                "\"properties\":{\"name\":\"");
        f.print(name);
        f.print("\",\"id\":");
        f.print(feat_count);
        f.print("},\"geometry\":");

        _geojson_write_polygon(f, cx, cy, rings, points_per_ring);

        f.print("}");

        written += est;
        feat_count++;

        // Laisser respirer le watchdog sur ESP8266/ESP32
        yield();
    }

    f.print("]}");
    f.flush();

    size_t actual = f.size();
    f.close();

    Serial.printf("[GeoJSON] %d features → %zu octets (%.1f KB) dans '%s'\n",
                  feat_count, actual, actual / 1024.0f, path);

    return (long)actual;
}
