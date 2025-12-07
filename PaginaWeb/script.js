// 1. CONFIGURACIÓN
const ipESP32 = "http://172.20.10.13"; 
const linkDrive = "https://drive.google.com/drive/folders/1BiKkWob-aSTng3TipAX-E18Wey-QfBQM?usp=sharing";

// Variable global para el intervalo de espera manual
let intervaloManual = null;

function cambiarNombre(nombreSeccion) 
{
    document.getElementById('titulo-tablero').innerText = nombreSeccion;
    for(let k=0; k<=3; k++) {
        let circulo = document.getElementById(`circulo-${k}`);
        if(circulo) {
            circulo.style.backgroundColor = ""; 
            circulo.className = "circulo"; 
            circulo.style.boxShadow = "none";
        }
    }

    let ruta = "";
    let saltoHuevos = 0; 
    if (nombreSeccion === 'SECCIÓN 1') { ruta = "/seccion1"; saltoHuevos = 0; } 
    else if (nombreSeccion === 'SECCIÓN 2') { ruta = "/seccion2"; saltoHuevos = 2; } 
    else if (nombreSeccion === 'SECCIÓN 3') { ruta = "/seccion3"; saltoHuevos = 6; }

    // PREPARAR INTERFAZ
    document.getElementById("contenedor-resultados").style.display = "none";
    const mensajeEstado = document.getElementById("mensaje-estado");
    mensajeEstado.style.display = "block";
    mensajeEstado.innerText = " Enviando orden...";
    mensajeEstado.style.color = "orange";

    // ENVIAR ORDEN DE INICIO
    fetch(ipESP32 + ruta)
        .then(response => {
            if (response.ok) {
                // Si el Arduino dijo "OK", empezamos a esperar los datos
                mensajeEstado.innerText = " Robot trabajando... (Espera)";
                empezarSondeoManual(nombreSeccion, saltoHuevos);
            } else {
                throw new Error("Arduino ocupado");
            }
        })
        .catch(error => {
            console.log("Error:", error);
            mensajeEstado.innerText = " Error de conexión";
            mensajeEstado.style.color = "red";
        });
}

// PREGUNTAR SI YA ACABÓ
function empezarSondeoManual(nombreSeccion, saltoHuevos) {
    if (intervaloManual) clearInterval(intervaloManual);

    intervaloManual = setInterval(() => {
        // Preguntamos a la nueva ruta 
        fetch(ipESP32 + "/pollManual")
            .then(res => res.text())
            .then(texto => {
                console.log("Estado Manual: " + texto);

                // Si dice ESPERA, seguimos esperando.
                // Si trae datos,  mostramos resultados.
                if (texto.includes(",")) {
                    clearInterval(intervaloManual);
                    mostrarResultadosManuales(texto, saltoHuevos, nombreSeccion);
                }
            })
            .catch(err => console.log("Esperando datos..."));
    }, 2000); // Preguntar cada 2 segundos
}

// MOSTRAR RESULTADOS
function mostrarResultadosManuales(textoRecibido, saltoHuevos, nombreSeccion) {
    const mensajeEstado = document.getElementById("mensaje-estado");
    const temperaturas = textoRecibido.split(',');

    mensajeEstado.style.display = "none"; 
    document.getElementById("contenedor-resultados").style.display = "block";
    document.getElementById("link-drive").href = linkDrive;

    // Limpieza
    for(let k=1; k<=10; k++) {
        let fila = document.getElementById(`fila-huevo-${k}`);
        if(fila) fila.style.display = "none";
    }

    // Llenado
    let suma = 0;
    let totalValidos = 0;

    for (let i = 0; i < temperaturas.length; i++) {
        let numeroHuevoReal = i + 1 + saltoHuevos; 
        let spanValor = document.getElementById(`valor-huevo-${numeroHuevoReal}`);
        let divFila = document.getElementById(`fila-huevo-${numeroHuevoReal}`);

        if (spanValor && divFila) {
            let valorTexto = temperaturas[i].trim();
            let valorNum = parseFloat(valorTexto);

            spanValor.innerText = valorTexto + " °C"; 
            divFila.style.display = "flex"; 

            if (!isNaN(valorNum)) {
                suma += valorNum;
                totalValidos++;
            }
        }
    }

    // Widgets
    if (totalValidos > 0) {
        let promedio = (suma / totalValidos).toFixed(1);
        let widgetTemp = document.getElementById('widget-temp');
        if(widgetTemp) widgetTemp.innerText = promedio;
        let widgetHum = document.getElementById('widget-hum');
        if(widgetHum) widgetHum.innerText = "55"; 
    }

    // Círculos
    if (nombreSeccion === 'SECCIÓN 1') {
        encenderCirculo('circulo-0'); 
        encenderCirculo('circulo-2'); 
    } else {
        encenderCirculo('circulo-0'); 
        encenderCirculo('circulo-1');
        encenderCirculo('circulo-2'); 
        encenderCirculo('circulo-3');
    }
}


// ==========================================
//      AUTOMATIZACIÓN 
// ==========================================

let alarmaActiva = false;
let horaProgramada = "";
let intervaloVerificacion = null; 

function activarAutomatico() {
    Swal.fire({
        title: 'Programar Inicio Automático',
        html: `
            <p style="color:white">Selecciona la hora de inicio:</p>
            <input type="time" id="hora-input" style="font-size: 20px; padding: 10px; border-radius: 10px;">
        `,
        background: 'rgba(20, 20, 30, 0.9)', 
        color: '#fff',
        showCancelButton: true,
        confirmButtonText: 'Programar',
        confirmButtonColor: '#03a9f4',
        cancelButtonText: 'Cancelar',
        preConfirm: () => {
            return document.getElementById('hora-input').value;
        }
    }).then((result) => {
        if (result.isConfirmed && result.value) {
            horaProgramada = result.value;
            alarmaActiva = true;
            
            Swal.fire({
                title: 'Programado',
                text: `El robot iniciará a las ${horaProgramada}`,
                icon: 'info',
                background: 'rgba(20, 20, 30, 0.9)',
                color: '#fff',
                timer: 3000,
                showConfirmButton: false
            });
        }
    });
}

// RELOJ INTERNO
setInterval(function() {
    if (alarmaActiva) {
        let fecha = new Date();
        let horaActual = ("0" + fecha.getHours()).slice(-2) + ":" + ("0" + fecha.getMinutes()).slice(-2);

        if (horaActual === horaProgramada) {
            console.log("¡Es la hora! Enviando orden al ESP32...");
            alarmaActiva = false; 
            
            enviarSenalInicio();
        }
    }
}, 1000);

function enviarSenalInicio() {
    fetch(ipESP32 + "/iniciar")
        .then(response => {
            if (response.ok) {
                console.log("Orden enviada correctamente");
                
                const Toast = Swal.mixin({
                    toast: true, position: 'top-end', showConfirmButton: false, timer: 3000,
                    background: '#03a9f4', color: '#fff'
                });
                Toast.fire({ icon: 'success', title: 'Robot Iniciado...' });

                iniciarSondeoTermino();
                
            } else {
                console.log("Error al enviar orden");
            }
        })
        .catch(error => console.log("Error de conexión: ", error));
}


function encenderCirculo(id) {
    let circulo = document.getElementById(id);
    if (circulo) {
        circulo.style.backgroundColor = "#39ff14"; 
        circulo.style.boxShadow = "0 0 15px #39ff14";
        circulo.style.borderColor = "#fff";
        circulo.style.transition = "all 0.5s ease";
    }
}

function encender(id) {
    let elemento = document.getElementById(id);
    if (elemento) {
        elemento.style.backgroundColor = "#39ff14"; 
        elemento.style.boxShadow = "0 0 15px #39ff14";
        elemento.style.borderColor = "#fff";
    }
}

function iniciarSondeoTermino() {
    if (intervaloVerificacion) clearInterval(intervaloVerificacion);

    intervaloVerificacion = setInterval(() => {
        
        fetch(ipESP32 + "/estado") 
            .then(response => response.text())
            .then(texto => {
                let estado = texto.trim();
                console.log("Progreso robot: " + estado);
                
                
                if (estado === "2" || estado === "3" || estado === "4" || estado === "FIN") {
                    encender("c1");
                    encender("c2");
                }

                if (estado === "3" || estado === "4" || estado === "FIN") {
                    encender("c3");
                    encender("c4");
                    encender("c5");
                    encender("c6");
                }

                if (estado === "4" || estado === "FIN") {
                    encender("c7");
                    encender("c8");
                    encender("c9");
                    encender("c10");
                }

                if (estado === "FIN") {
                    clearInterval(intervaloVerificacion); 
                    intervaloVerificacion = null;
                    
                    Swal.fire({
                        title: '¡Ciclo Terminado!',
                        text: 'El proceso automático ha finalizado con éxito.',
                        icon: 'success',
                        background: 'rgba(20, 20, 30, 0.9)',
                        color: '#fff'
                    });
                }
            })
            .catch(err => console.log("Esperando conexión..."));

    }, 1000); 
}


// ==========================================
//      ACTUALIZACIÓN AUTOMÁTICA DE WIDGETS
// ==========================================

function actualizarAmbiente() {
    console.log("Actualizando widgets de ambiente...");
    
    // Llamamos a la nueva ruta 
    fetch(ipESP32 + "/ambiente")
        .then(response => {
            if (!response.ok) throw new Error("Error ESP32 Ambiente");
            return response.text();
        })
        .then(texto => {
            const datos = texto.split(',');
            
            if (datos.length >= 2) {
                // Actualizar Widget Temperatura
                let wTemp = document.getElementById('widget-temp');
                if(wTemp) wTemp.innerText = datos[0];

                // Actualizar Widget Humedad
                let wHum = document.getElementById('widget-hum');
                if(wHum) wHum.innerText = datos[1];
            }
        })
        .catch(error => console.log("Error actualizando ambiente:", error));
}

actualizarAmbiente();

// Ejecutar cada 60 segundos (60000 milisegundos)
setInterval(actualizarAmbiente, 60000);