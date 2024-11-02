var list1 = document.getElementById('shared-list-1');
var list2 = document.getElementById('shared-list-2');

// Fonction pour envoyer l'ordre de la liste 2 au serveur
function sendList2Order() {
    var order = [];
       // Sélectionnez tous les éléments de la liste 2
    list2.querySelectorAll('.list-group-item').forEach(function (item) {
        // Récupérez l'ID (data-id) de l'élément
        const id = item.getAttribute('data-id');
        order.push(id); // Ajoutez cet ID à l'ordre
    });

    // Envoyer l'ordre de la liste 2 au serveur ESP32 via une requête POST
    fetch('/update-order', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ order: order })
    })
    .then(response => response.json())
    .then(data => console.log('Success:', data))
    .catch((error) => {
        console.error('Error:', error);
    });
}

// Initialiser Sortable pour la liste 1
new Sortable(list1, {
    group: 'shared',
    animation: 150,
    onEnd: function (evt) {
        sendList2Order(); // Envoyer l'ordre de la liste 2 après l'interaction avec la liste 1
    }
});

// Initialiser Sortable pour la liste 2
new Sortable(list2, {
    group: 'shared',
    animation: 150,
    onEnd: function (evt) {
        sendList2Order(); // Envoyer l'ordre de la liste 2 après l'interaction avec la liste 2
    }
});
 // Ouvrir la modale
 function openModal() {
    document.getElementById("settings-modal").style.display = "block";
}

// Fermer la modale
function closeModal() {
    document.getElementById("settings-modal").style.display = "none";
}


function sendAboutVersionCommand() {
    fetch('/send-about-version', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ command: 'AboutVersion' })
    })
    .then(response => response.json())
    .then(data => {
        console.log('Success:', data);
    })
    .catch((error) => {
        console.error('Error:', error);
    });
}


let currentValue = 1;

function changeValue(amount) {
    currentValue += amount;

    // Limite les valeurs entre 1 et 16
    if (currentValue < 1) currentValue = 1;
    if (currentValue > 16) currentValue = 16;

    // Met à jour l'affichage
    document.getElementById('value-display').innerText = currentValue;
}

// Fonction pour envoyer la commande de changement de preset
function sendChangePresetCommand() {
    const command = `hw sendrs change preset ${currentValue}`;
    console.log(command); // Affiche la commande dans la console pour vérification

    // Envoyer la requête POST à la route "/change-preset"
    fetch("/change-preset", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded",
        },
        body: `preset=${currentValue}`, // Envoie la valeur du preset
    })
    .then(response => response.json())
    .then(data => {
        console.log(data); // Affiche la réponse du serveur
    })
    .catch((error) => {
        console.error("Error:", error);
    });
}