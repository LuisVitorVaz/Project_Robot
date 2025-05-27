const canvas = document.getElementById("smileyCanvas");
const ctx = canvas.getContext("2d");
const input = document.getElementById("expressionInput");

// Variáveis para controlar a animação e a expressão
let angle = 0;
let mouthOpen = 0;
let mouthDirection = 1;
let invertedEyes = false; // Estado dos olhos

// Função para desenhar os olhos
function drawEyes() {

    // Centro e posicionamento dos olhos
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const eyeOffsetX = 60; // Distância entre os olhos
    const eyeOffsetY = 20; // Posição vertical dos olhos
    const eyeRadius = 30;  // Tamanho do olho (branco)
    const pupilRadius = 15; // Tamanho da pupila
    const pupilOffsetX = invertedEyes ? -10 : 10; // Movimento da pupila
    const shineRadius = 5;  // Tamanho do brilho

    // Desenha olho esquerdo (branco)
    ctx.beginPath();
    ctx.ellipse(centerX - eyeOffsetX, centerY - eyeOffsetY, eyeRadius, eyeRadius * 1.5, 0, 0, Math.PI * 2);
    ctx.fillStyle = "white";
    ctx.fill();
    ctx.stroke();

    // Desenha pupila do olho esquerdo
    ctx.beginPath();
    ctx.arc(centerX - eyeOffsetX + pupilOffsetX, centerY - eyeOffsetY, pupilRadius, 0, Math.PI * 2);
    ctx.fillStyle = "black";
    ctx.fill();

    // Brilho no olho esquerdo
    ctx.beginPath();
    ctx.arc(centerX - eyeOffsetX + pupilOffsetX + 5, centerY - eyeOffsetY - 5, shineRadius, 0, Math.PI * 2);
    ctx.fillStyle = "white";
    ctx.fill();

    // Desenha olho direito (branco)
    ctx.beginPath();
    ctx.ellipse(centerX + eyeOffsetX, centerY - eyeOffsetY, eyeRadius, eyeRadius * 1.5, 0, 0, Math.PI * 2);
    ctx.fillStyle = "white";
    ctx.fill();
    ctx.stroke();

    // Desenha pupila do olho direito
    ctx.beginPath();
    ctx.arc(centerX + eyeOffsetX + pupilOffsetX, centerY - eyeOffsetY, pupilRadius, 0, Math.PI * 2);
    ctx.fillStyle = "black";
    ctx.fill();

    // Brilho no olho direito
    ctx.beginPath();
    ctx.arc(centerX + eyeOffsetX + pupilOffsetX + 5, centerY - eyeOffsetY - 5, shineRadius, 0, Math.PI * 2);
    ctx.fillStyle = "white";
    ctx.fill();
}

function drawMouth() {
    
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    
    // Outer black stroke
    ctx.beginPath();
    ctx.moveTo(centerX - 70, centerY + 60);
    ctx.lineTo(centerX - 80, centerY + 55);
    ctx.bezierCurveTo(
        centerX - 40, centerY + 40 - (mouthOpen * 20),
        centerX + 40, centerY + 40 - (mouthOpen * 20),
        centerX + 80, centerY + 55
    );
    ctx.lineTo(centerX + 70, centerY + 60);
    ctx.bezierCurveTo(
        centerX + 40, centerY + 80 + (mouthOpen * 20),
        centerX - 40, centerY + 80 + (mouthOpen * 20),
        centerX - 70, centerY + 60
    );
    ctx.lineWidth = 3;
    ctx.strokeStyle = 'black';
    ctx.stroke();
    ctx.closePath();
    
    // Inner curve
    ctx.beginPath();
    ctx.moveTo(centerX - 60, centerY + 65);
    ctx.bezierCurveTo(
        centerX - 30, centerY + 60 + (mouthOpen * 15),
        centerX + 30, centerY + 60 + (mouthOpen * 15),
        centerX + 60, centerY + 65
    );
    ctx.lineWidth = 2;
    ctx.strokeStyle = 'black';
    ctx.stroke();
    ctx.closePath();
    
    // Update animation
    mouthOpen += 0.02 * mouthDirection;
    if (mouthOpen > 1 || mouthOpen < 0) {
        mouthDirection *= -1;
    }
}

// Função para desenhar a carinha
function drawSmiley() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Centro e tamanho do rosto
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const radius = 150;

    // Rosto (círculo)
    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0, Math.PI * 2, true);
    ctx.fillStyle = "yellow";
    ctx.fill();
    ctx.stroke();

    drawEyes(); // Desenha os olhos
    drawMouth(); // Desenha a boca
}

// Loop de animação
function animate() {
    drawSmiley();
    requestAnimationFrame(animate);
}

// Evento para mudar a expressão ao digitar
input.addEventListener("input", () => {
    if (input.value.toLowerCase() === "ola") {
        invertedEyes = true; // Muda para expressão invertida
        mouthOpen = 1; // Abre a boca totalmente
        mouthDirection = -1; // Começa fechando
    } else {
        invertedEyes = false; // Retorna à expressão normal
        mouthOpen = 0; // Fecha a boca
        mouthDirection = 1; // Volta à direção original
    }
});

// Iniciar animação
animate();
