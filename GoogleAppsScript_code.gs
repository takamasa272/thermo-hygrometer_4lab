function doGet(e) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheets()[0]

  const params = {
  "timestamp": new Date(),
  "temperature": e.parameter.temperature,
  "humidity": e.parameter.humidity
  };

  sheet.appendRow(Object.values(params));
  return ContentService.createTextOutput('sccess');
}
