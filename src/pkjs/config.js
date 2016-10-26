module.exports = [
  {
    "type": "heading",
    "defaultValue": "App Configuration"
  },
  {
    "type": "text",
    "defaultValue": "Here is some introductory text."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background Color"
      },
      {
        "type": "color",
        "messageKey": "ForegroundColor",
        "defaultValue": "0xFFFFFF",
        "label": "Foreground Color"
      }
    ]
  },

  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "More Settings"
      },
      {
        "type": "toggle",
        "messageKey": "SecondTick",
        "label": "Enable Seconds",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "Animations",
        "label": "Enable Animations",
        "defaultValue": false
      },
      {
  "type": "input",
  "messageKey": "email",
  "defaultValue": "",
  "label": "Nightscout Website",
  "attributes": {
    "placeholder": "eg: https://yourname.azurewebsites.net",
    //"limit": 10,
    "type": "email"
  }
},
  "type": "input",
  "messageKey": "t1name",
  "defaultValue": "",
      "label": "Enter Initial or IOB:",
  "attributes": {
    "placeholder": " ",
    //"limit": 10,
    "type": "text"
  }
},
      
      {
  "type": "radiogroup",
  "messageKey": "favorite_food",
  "label": "Choose Units:",
  "options": [
    { 
      "label": "mmol/L", 
      "value": "mmol" 
    },
    { 
      "label": "mg/dL", 
      "value": "mgdl" 
    },
    
  ]
},
      {
  "type": "toggle",
  "messageKey": "invert",
  "label": "Invert Colors",
  "defaultValue": true
},
      {
  "type": "select",
  "messageKey": "mode",
  "defaultValue": "nightscout",
  "label": "Select Mode",
  "options": [
    { 
      "label": "", 
      "value": "" 
    },
    { 
      "label": "Nighstcout",
      "value": "Nightscout" 
    },
    { 
      "label": "US Share",
      "value": "US_Share" 
    },
    { 
      "label": "Non-US Share",
      "value": "Non_US_Share" 
    }
  ]
}
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];