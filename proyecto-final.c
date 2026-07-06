/* SIGPCA - Gestion y Prediccion de Contaminacion del Aire (C99, sin libs externas) */
#include <stdio.h>
#include <string.h>
#define MAX_ZONAS 5
#define DIAS 30
#define ARCH_DATOS "sigpca_datos.dat"
#define ARCH_REPORTE "sigpca_reporte.txt"
#define BUENO 0
#define MODERADO 1
#define DANINO 2
#define PELIGROSO 3
/* Contaminantes: 0=CO2 1=SO2 2=NO2 3=PM2.5 */
static const char *POL[4] = {"CO2","SO2","NO2","PM2.5"};
static const float LIM[4][3] = { {50,100,200}, {20,50,125}, {40,100,200}, {15,35,75} };
typedef struct { float v[4]; } Contaminacion;
typedef struct { float temp, viento, dirViento, humedad; } Clima;
typedef struct {
    int id;
    char nombre[40];
    Contaminacion actual, prediccion;
    Contaminacion historico[DIAS];
    Clima clima;
    int nivelAlerta;
} Zona;
void inicializarZonas(Zona z[], int n) {
    const char *nom[MAX_ZONAS] = {"Zona Centro","Zona Norte","Zona Sur","Zona Industrial","Zona Residencial"};
    int i, d, k;
    for (i = 0; i < n; i++) {
        z[i].id = i + 1;
        strncpy(z[i].nombre, nom[i], sizeof(z[i].nombre) - 1);
        z[i].nombre[sizeof(z[i].nombre) - 1] = '\0';
        for (k = 0; k < 4; k++) { z[i].actual.v[k] = 0; z[i].prediccion.v[k] = 0; }
        for (d = 0; d < DIAS; d++)
            for (k = 0; k < 4; k++) z[i].historico[d].v[k] = 0;
        memset(&z[i].clima, 0, sizeof(Clima));
        z[i].nivelAlerta = BUENO;
    }
}
int cargarDatos(Zona z[], int n) {
    FILE *f = fopen(ARCH_DATOS, "rb");
    size_t leidos;
    if (!f) return 0;
    leidos = fread(z, sizeof(Zona), (size_t) n, f);
    fclose(f);
    return leidos == (size_t) n;
}
void guardarDatos(const Zona z[], int n) {
    FILE *f = fopen(ARCH_DATOS, "wb");
    if (!f) { printf("Error al guardar datos.\n"); return; }
    fwrite(z, sizeof(Zona), (size_t) n, f);
    fclose(f);
}
void listarZonas(const Zona z[], int n) {
    int i;
    printf("--- Zonas registradas ---\n");
    for (i = 0; i < n; i++)
        printf("%d. %s (Nivel: %s)\n", z[i].id, z[i].nombre,
               (char*[]){"Bueno","Moderado","Danino","Peligroso"}[z[i].nivelAlerta]);
}
int seleccionarZona(const Zona z[], int n) {
    int i, id;
    listarZonas(z, n);
    printf("Ingrese el numero de la zona: ");
    if (scanf("%d", &id) != 1) { while (getchar() != '\n'); return -1; }
    for (i = 0; i < n; i++) if (z[i].id == id) return i;
    printf("Zona no encontrada.\n");
    return -1;
}
void ingresarDatosActuales(Zona *z) {
    int k;
    printf("--- Datos actuales para %s ---\n", z->nombre);
    for (k = 0; k < 4; k++) {
        printf("%s: ", POL[k]);
        scanf("%f", &z->actual.v[k]);
        if (z->actual.v[k] < 0) z->actual.v[k] = 0;
    }
    printf("Temperatura (C): "); scanf("%f", &z->clima.temp);
    printf("Viento (m/s): ");    scanf("%f", &z->clima.viento);
    printf("Direccion viento (grados): "); scanf("%f", &z->clima.dirViento);
    printf("Humedad (%%): ");    scanf("%f", &z->clima.humedad);
}
/* Desplaza el historico e inserta el registro actual como el dia mas reciente */
void actualizarHistorico(Zona *z) {
    int d;
    for (d = 0; d < DIAS - 1; d++) z->historico[d] = z->historico[d + 1];
    z->historico[DIAS - 1] = z->actual;
}
/* Prediccion 24h = 50% actual + 30% promedio ultimos 7 dias + 20% promedio mensual */
void calcularPrediccion(Zona *z) {
    int d, k;
    float sumaMes[4] = {0}, sumaSem[4] = {0};
    for (d = 0; d < DIAS; d++)
        for (k = 0; k < 4; k++) {
            sumaMes[k] += z->historico[d].v[k];
            if (d >= DIAS - 7) sumaSem[k] += z->historico[d].v[k];
        }
    for (k = 0; k < 4; k++)
        z->prediccion.v[k] = 0.5f * z->actual.v[k] + 0.3f * (sumaSem[k] / 7.0f) + 0.2f * (sumaMes[k] / DIAS);
}
int nivelContaminante(float valor, const float lim[3]) {
    if (valor <= lim[0]) return BUENO;
    if (valor <= lim[1]) return MODERADO;
    if (valor <= lim[2]) return DANINO;
    return PELIGROSO;
}
/* Compara valores actuales y predichos contra los limites OMS; conserva el peor nivel */
void evaluarAlerta(Zona *z) {
    int k, nivel, peor = BUENO;
    for (k = 0; k < 4; k++) {
        nivel = nivelContaminante(z->actual.v[k], LIM[k]);
        if (nivel > peor) peor = nivel;
        nivel = nivelContaminante(z->prediccion.v[k], LIM[k]);
        if (nivel > peor) peor = nivel;
    }
    z->nivelAlerta = peor;
}
void mostrarRecomendacion(int nivel) {
    const char *msj[4] = {
        "Calidad del aire aceptable. No se requieren medidas especiales.",
        "Grupos sensibles deben reducir actividad fisica prolongada al aire libre.",
        "Evitar actividades al aire libre, usar mascarilla y restringir circulacion vehicular.",
        "Alerta critica: permanecer en interiores, suspender actividades exteriores y aplicar\nrestriccion vehicular de emergencia."
    };
    printf("--- Recomendacion ---\n%s\n", msj[nivel]);
}
void mostrarEstadoZona(const Zona *z) {
    int k;
    printf("--- Estado de %s ---\nActual:      ", z->nombre);
    for (k = 0; k < 4; k++) printf("%s=%.2f ", POL[k], z->actual.v[k]);
    printf("\nPrediccion 24h: ");
    for (k = 0; k < 4; k++) printf("%s=%.2f ", POL[k], z->prediccion.v[k]);
    printf("\nNivel de alerta: %s\n", (char*[]){"Bueno","Moderado","Danino","Peligroso"}[z->nivelAlerta]);
}
void generarReporte(const Zona z[], int n) {
    FILE *f = fopen(ARCH_REPORTE, "w");
    int i, k;
    if (!f) { printf("Error al generar reporte.\n"); return; }
    fprintf(f, "REPORTE SIGPCA - Contaminacion del Aire en Zonas Urbanas\n============================================\n");
    for (i = 0; i < n; i++) {
        fprintf(f, "\nZona: %s (ID %d)\n  Actual: ", z[i].nombre, z[i].id);
        for (k = 0; k < 4; k++) fprintf(f, "%s=%.2f ", POL[k], z[i].actual.v[k]);
        fprintf(f, "\n  Prediccion 24h: ");
        for (k = 0; k < 4; k++) fprintf(f, "%s=%.2f ", POL[k], z[i].prediccion.v[k]);
        fprintf(f, "\n  Nivel de alerta: %s\n",
                (char*[]){"Bueno","Moderado","Danino","Peligroso"}[z[i].nivelAlerta]);
    }
    fclose(f);
}
int main(void) {
    Zona zonas[MAX_ZONAS];
    int opcion, idx;
    if (!cargarDatos(zonas, MAX_ZONAS)) {
        inicializarZonas(zonas, MAX_ZONAS);
        printf("Sin archivo previo. Se inicializaron %d zonas por defecto.\n", MAX_ZONAS);
    }
    do {
        printf("\n=== SIGPCA - Menu ===\n1.Listar 2.Ingresar datos 3.Predecir/Alertar 4.Reporte+Guardar 5.Salir\nOpcion: ");
        if (scanf("%d", &opcion) != 1) { while (getchar() != '\n'); opcion = -1; }
        switch (opcion) {
            case 1: listarZonas(zonas, MAX_ZONAS); break;
            case 2:
                idx = seleccionarZona(zonas, MAX_ZONAS);
                if (idx >= 0) ingresarDatosActuales(&zonas[idx]);
                break;
            case 3:
                idx = seleccionarZona(zonas, MAX_ZONAS);
                if (idx >= 0) {
                    calcularPrediccion(&zonas[idx]);
                    evaluarAlerta(&zonas[idx]);
                    actualizarHistorico(&zonas[idx]);
                    mostrarEstadoZona(&zonas[idx]);
                    mostrarRecomendacion(zonas[idx].nivelAlerta);
                }
                break;
            case 4:
                generarReporte(zonas, MAX_ZONAS);
                guardarDatos(zonas, MAX_ZONAS);
                printf("Reporte y datos guardados.\n");
                break;
            case 5:
                guardarDatos(zonas, MAX_ZONAS);
                printf("Saliendo...\n");
                break;
            default:
                printf("Opcion invalida.\n");
        }
    } while (opcion != 5);
    return 0;
}