function doPost(e) {
  const SPREADSHEET_ID = "12UQ9h8TqpYO6ImsLQzr6-_rSdpfSNx96pdOr0kG0ms8";
  const ss = SpreadsheetApp.openById(SPREADSHEET_ID);

  if (!e.postData || !e.postData.contents) {
    return ContentService
      .createTextOutput("No hay datos en POST")
      .setMimeType(ContentService.MimeType.TEXT);
  }

  const jsonData = JSON.parse(e.postData.contents);

  // =====================================================
  // ===============   CASO 1: FOTO   ====================
  // =====================================================
  if (jsonData.imageData) {
    const SHEET_NAME = "Hoja 1";
    const sheet = ss.getSheetByName(SHEET_NAME);

    // Nombre de la foto
    const imageName =
      Utilities.formatDate(new Date(), "GMT-6", "yyyyMMdd-HHmmss") + ".jpg";

    // Carpeta destino
    const folderName =
      (e.parameter && e.parameter.folder) ? e.parameter.folder : "ESP32-CAM";
    const savingFolder = getFolder(folderName);

    // Procesar Base64
    let base64Image = jsonData.imageData;
    let base64Data = base64Image.replace(/^data:image\/\w+;base64,/, "");
    const decodedImage = Utilities.base64Decode(base64Data);

    // Crear archivo en Drive
    const blob = Utilities.newBlob(decodedImage, "image/jpeg", imageName);
    const file = savingFolder.createFile(blob);
    file.setSharing(DriveApp.Access.ANYONE_WITH_LINK, DriveApp.Permission.VIEW);

    const viewUrl =
      "https://drive.google.com/uc?export=view&id=" + file.getId();

    // ======== ASIGNAR TEXTO SEGÚN LA SECCIÓN ========
    let nombreEnHoja = folderName;

    if (jsonData.seccion !== undefined) {
      const s = jsonData.seccion;
      if (s == 1) nombreEnHoja = "Seccion 1";
      else if (s == 2) nombreEnHoja = "Seccion 2";
      else if (s == 3) nombreEnHoja = "Seccion 3";
      else nombreEnHoja = "Seccion " + s;
    }

    // ===== Escribir fila nueva =====
    const fila = sheet.getLastRow() + 1;

    sheet.getRange(fila, 1).setValue(new Date());            // A: fecha
    sheet.getRange(fila, 2).setValue(nombreEnHoja);          // B: sección
    sheet.getRange(fila, 3).setValue(folderName);            // C: carpeta
    sheet.getRange(fila, 4).setValue(viewUrl);               // D: URL
    sheet.getRange(fila, 5).setFormula('=IMAGE("' + viewUrl + '")'); // E
    // F: temperatura después

    // ================================
    // CONTADOR DE FOTOS + ENVÍO CORREO
    // ================================
    const LIMITE_FOTOS = 10;

    const props = PropertiesService.getScriptProperties();
    let contador = parseInt(props.getProperty('contador_fotos') || '0', 10);

    contador++;
    props.setProperty('contador_fotos', String(contador));

    if (contador >= LIMITE_FOTOS) {
      enviarSheetPorCorreo();
      props.setProperty('contador_fotos', '0');
    }

    return ContentService
      .createTextOutput("OK foto")
      .setMimeType(ContentService.MimeType.TEXT);
  }

  // =====================================================
  // ===========   CASO 2: TEMPERATURA GY-906   ==========
  // =====================================================
  if (jsonData.tempC !== undefined) {
    const SHEET_NAME = "Hoja 1";
    const sheet = ss.getSheetByName(SHEET_NAME);
    const tempC = jsonData.tempC;

    // ======== ASIGNAR TEXTO SEGÚN LA SECCIÓN ========
    let etiquetaSeccion = null;

    if (jsonData.seccion !== undefined) {
      const s = jsonData.seccion;
      if (s == 1) etiquetaSeccion = "Seccion 1";
      else if (s == 2) etiquetaSeccion = "Seccion 2";
      else if (s == 3) etiquetaSeccion = "Seccion 3";
      else etiquetaSeccion = "Seccion " + s;
    }

    let filaDestino = null;
    const lastRow = sheet.getLastRow();

    if (etiquetaSeccion && lastRow >= 2) {

      const values = sheet.getRange(2, 1, lastRow - 1, 6).getValues();
      // [A,B,C,D,E,F]

      for (let i = values.length - 1; i >= 0; i--) {
        const fila = values[i];
        const seccionFila = fila[1]; // columna B
        const tempExistente = fila[5]; // columna F

        if (seccionFila === etiquetaSeccion &&
            (tempExistente === "" || tempExistente === null)) {

          filaDestino = i + 2; // fila real en sheet
          break;
        }
      }
    }

    if (filaDestino) {
      // Emparejar con foto
      sheet.getRange(filaDestino, 6).setValue(tempC);   // F: temperatura
      sheet.getRange(filaDestino, 1).setValue(new Date()); // actualizar hora
    } else {
      // Crear nueva fila si no encuentra
      const nuevaFila = sheet.getLastRow() + 1;
      sheet.getRange(nuevaFila, 1).setValue(new Date());     // A
      if (etiquetaSeccion) sheet.getRange(nuevaFila, 2).setValue(etiquetaSeccion); // B
      sheet.getRange(nuevaFila, 6).setValue(tempC);           // F
    }

    return ContentService
      .createTextOutput("OK temp")
      .setMimeType(ContentService.MimeType.TEXT);
  }

  return ContentService
    .createTextOutput("POST sin imageData ni tempC")
    .setMimeType(ContentService.MimeType.TEXT);
}


// ----------------------------------------------
// Buscar o crear carpeta
// ----------------------------------------------
function getFolder(folderName) {
  const folders = DriveApp.getFoldersByName(folderName);
  return folders.hasNext()
    ? folders.next()
    : DriveApp.createFolder(folderName);
}

// ----------------------------------------------
// ENVIAR EL SHEET POR CORREO COMO PDF
// ----------------------------------------------
function enviarSheetPorCorreo() {
  const SPREADSHEET_ID = "12UQ9h8TqpYO6ImsLQzr6-_rSdpfSNx96pdOr0kG0ms8"; 
  const ss = SpreadsheetApp.openById(SPREADSHEET_ID);

  const pdf = ss.getAs("application/pdf");

  const destinatarios = "A01712297@tec.mx";

  const fechaTexto = Utilities.formatDate(
    new Date(),
    Session.getScriptTimeZone(),
    "yyyy-MM-dd HH:mm"
  );

  const asunto = "Reporte incubadora " + fechaTexto;
  const cuerpo =
    "Hola,\n\nTe envío el reporte automático de la hoja de la incubadora.\n\nSaludos.";

  MailApp.sendEmail({
    to: destinatarios,
    subject: asunto,
    body: cuerpo,
    attachments: [pdf]
  });
}
