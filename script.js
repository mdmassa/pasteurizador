// Atualizar dados a cada 5 segundos
setInterval(() => {
    fetch("/")
        .then(response => response.text())
        .then(data => {
            const parser = new DOMParser();
            const doc = parser.parseFromString(data, "text/html");
            
            // Atualizar temperaturas
            const tempValues = doc.querySelectorAll('.temp-value');
            document.querySelectorAll('.temp-value').forEach((el, index) => {
                if (tempValues[index]) {
                    el.textContent = tempValues[index].textContent;
                }
            });
            
            // Atualizar estados (aquecimento/resfriamento)
            const statusHeating = doc.querySelector('.heating')?.textContent.includes("Aquecimento");
            const statusCooling = doc.querySelector('.cooling')?.textContent.includes("Resfriamento");
            
            if (statusHeating !== undefined) {
                document.querySelector('.heating').textContent = statusHeating ? "Aquecimento" : "Inativo";
            }
            
            if (statusCooling !== undefined) {
                document.querySelector('.cooling').textContent = statusCooling ? "Resfriamento" : "Inativo";
            }
        });
}, 5000);

// Controles
document.querySelector('.stop-btn').addEventListener('click', () => {
    fetch("/controle", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded",
        },
        body: "dispositivo=all&acao=parar"
    });
});

document.querySelector('.clean-btn').addEventListener('click', () => {
    fetch("/controle", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded",
        },
        body: "dispositivo=all&acao=limpeza"
    });
});