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

// Fonction pour envoyer la commande de reset
function sendResetCommand() {
    fetch("/reset", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ command: "reset" })
    })
    .then(response => response.json())
    .then(data => console.log("Reset command sent:", data))
    .catch((error) => console.error("Error:", error));
}

// Fonctions pour la modal d'ajout d'effet
function openAddEffectModal() {
    document.getElementById("add-effect-modal").style.display = "block";
}

function closeAddEffectModal() {
    document.getElementById("add-effect-modal").style.display = "none";
}

async function searchImages() {
    const searchTerm = document.getElementById("image-search").value;
    const resultsContainer = document.getElementById("search-results");
    resultsContainer.innerHTML = "Recherche en cours...";

    try {
        // Utilisation de l'API Unsplash (vous devrez obtenir une clé API)
        const response = await fetch(`https://api.unsplash.com/search/photos?query=${searchTerm}&client_id=VOTRE_CLE_API`);
        const data = await response.json();

        resultsContainer.innerHTML = "";
        data.results.forEach(image => {
            const div = document.createElement("div");
            div.className = "search-result-item";
            div.innerHTML = `<img src="${image.urls.small}" alt="${image.alt_description}">`;
            div.onclick = () => addImageToRack(image.urls.regular);
            resultsContainer.appendChild(div);
        });
    } catch (error) {
        resultsContainer.innerHTML = "Erreur lors de la recherche d'images";
        console.error("Error:", error);
    }
}

function addImageFromUrl() {
    const url = document.getElementById("manual-url").value;
    if (url) {
        addImageToRack(url);
    }
}

// Gestion du drag and drop
function setupDragAndDrop() {
    const dropZone = document.getElementById('drop-zone');

    // Empêcher le comportement par défaut
    ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
        dropZone.addEventListener(eventName, preventDefaults, false);
        document.body.addEventListener(eventName, preventDefaults, false);
    });

    // Gestion des effets visuels
    ['dragenter', 'dragover'].forEach(eventName => {
        dropZone.addEventListener(eventName, highlight, false);
    });

    ['dragleave', 'drop'].forEach(eventName => {
        dropZone.addEventListener(eventName, unhighlight, false);
    });

    // Gestion du drop
    dropZone.addEventListener('drop', handleDrop, false);
}

function preventDefaults(e) {
    e.preventDefault();
    e.stopPropagation();
}

function highlight(e) {
    document.getElementById('drop-zone').classList.add('drag-over');
}

function unhighlight(e) {
    document.getElementById('drop-zone').classList.remove('drag-over');
}

function handleDrop(e) {
    const dt = e.dataTransfer;
    const files = dt.files;

    handleFiles(files);
}

function handleFiles(files) {
    const file = files[0]; // On ne gère qu'un fichier à la fois
    
    if (file && file.type.startsWith('image/')) {
        const reader = new FileReader();
        
        reader.onload = function(e) {
            addImageToRack(e.target.result);
        }
        
        reader.readAsDataURL(file);
    } else {
        alert('Veuillez déposer une image valide');
    }
}

function addImageToRack(imageUrl) {
    const list = document.getElementById("shared-list-1");
    const newId = list.children.length; // Générer un nouvel ID
    
    const newEffect = document.createElement("div");
    newEffect.className = "list-group-item";
    newEffect.setAttribute("data-id", newId);
    newEffect.innerHTML = `
        <span class="element-number">${newId}</span>
        <img src="${imageUrl}" alt="Effect ${newId}">
    `;
    
    // Insérer avant la zone de drop
    list.insertBefore(newEffect, list.lastElementChild);
}

// Initialiser le drag and drop au chargement de la page
document.addEventListener('DOMContentLoaded', setupDragAndDrop);

// Fermer la modal si on clique en dehors
window.onclick = function(event) {
    const addEffectModal = document.getElementById("add-effect-modal");
    if (event.target === addEffectModal) {
        closeAddEffectModal();
    }
}

// Ajouter cette fonction
function resetLayout() {
    // Récupérer tous les éléments du rack de droite
    const rightRack = document.getElementById('shared-list-2');
    const leftRack = document.getElementById('shared-list-1');
    
    // Stocker temporairement tous les éléments dans un tableau
    const allElements = [];
    
    // Récupérer les éléments du rack de droite
    while (rightRack.children.length > 0) {
        allElements.push(rightRack.children[0]);
        rightRack.children[0].remove();
    }
    
    // Récupérer les éléments du rack de gauche (sauf la zone de drop)
    const dropZone = leftRack.querySelector('.drop-zone');
    while (leftRack.children.length > 1) { // > 1 car on garde la zone de drop
        allElements.push(leftRack.children[0]);
        leftRack.children[0].remove();
    }
    
    // Trier les éléments par leur data-id
    allElements.sort((a, b) => {
        const idA = parseInt(a.getAttribute('data-id'));
        const idB = parseInt(b.getAttribute('data-id'));
        return idA - idB;
    });
    
    // Remettre tous les éléments dans le rack de gauche dans l'ordre
    allElements.forEach(element => {
        leftRack.insertBefore(element, dropZone);
    });

    // Envoyer l'ordre vide au serveur pour mettre à jour les LEDs
    fetch('/update-order', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ order: [] })
    })
    .then(response => response.json())
    .then(data => console.log('Reset success:', data))
    .catch(error => console.error('Reset error:', error));
}
