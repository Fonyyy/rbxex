package com.rbxex.app

import android.content.ClipData
import android.content.ClipboardManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import kotlinx.coroutines.*

class MainActivity : AppCompatActivity() {

    private lateinit var editorView: EditText
    private lateinit var executeButton: Button
    private lateinit var clearButton: Button
    private lateinit var statusText: TextView
    private lateinit var statusDot: View
    private lateinit var outputText: TextView
    private lateinit var tabContainer: LinearLayout

    private val mainScope = CoroutineScope(Dispatchers.Main + SupervisorJob())
    private val statusHandler = Handler(Looper.getMainLooper())
    private val statusRunnable = object : Runnable {
        override fun run() {
            updateStatus()
            statusHandler.postDelayed(this, 1000)
        }
    }

    // Pre-built script templates
    private val templates = listOf(
        ScriptTemplate("Print Test", "print(\"Hello from RbxEx!\")"),
        ScriptTemplate("LocalPlayer", """
local player = game.Players.LocalPlayer
print("Player: " .. player.Name)
print("UserId: " .. player.UserId)
""".trimIndent()),
        ScriptTemplate("Walk Speed", """
game.Players.LocalPlayer.Character.Humanoid.WalkSpeed = 100
""".trimIndent()),
        ScriptTemplate("Jump Power", """
game.Players.LocalPlayer.Character.Humanoid.JumpPower = 200
""".trimIndent()),
        ScriptTemplate("Infinite Jump", """
local UIS = game:GetService("UserInputService")
local jumping = false
game.Players.LocalPlayer.Character.Humanoid.StateChanged:Connect(function(old, new)
    if new == Enum.HumanoidStateType.Jumping then
        jumping = true
    elseif new == Enum.HumanoidStateType.Landed then
        jumping = false
    end
end)
UIS.JumpRequest:Connect(function()
    if jumping then
        game.Players.LocalPlayer.Character.Humanoid:ChangeState(Enum.HumanoidStateType.Jumping)
    end
end)
print("Infinite jump enabled!")
""".trimIndent()),
        ScriptTemplate("Fly Script", """
local player = game.Players.LocalPlayer
local char = player.Character
local hum = char:WaitForChild("Humanoid")
local hrp = char:WaitForChild("HumanoidRootPart")

local flying = false
local speed = 50
local bv = Instance.new("BodyVelocity", hrp)
bv.Velocity = Vector3.new(0, 0, 0)
bv.MaxForce = Vector3.new(1e5, 1e5, 1e5)

local bg = Instance.new("BodyGyro", hrp)
bg.MaxTorque = Vector3.new(1e5, 1e5, 1e5)
bg.P = 1e4

game:GetService("RunService").RenderStepped:Connect(function()
    if flying then
        local cam = workspace.CurrentCamera
        bv.Velocity = cam.CFrame.LookVector * speed
        bg.CFrame = cam.CFrame
    end
end)

game:GetService("UserInputService").InputBegan:Connect(function(input)
    if input.KeyCode == Enum.KeyCode.F then
        flying = not flying
        if not flying then bv.Velocity = Vector3.zero end
        print(flying and "Flying!" or "Landing...")
    end
end)
print("Fly enabled! Press F to toggle.")
""".trimIndent()),
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        initViews()
        setupTemplates()
        setupButtons()
        statusHandler.post(statusRunnable)
    }

    private fun initViews() {
        editorView    = findViewById(R.id.editor)
        executeButton = findViewById(R.id.btn_execute)
        clearButton   = findViewById(R.id.btn_clear)
        statusText    = findViewById(R.id.status_text)
        statusDot     = findViewById(R.id.status_dot)
        outputText    = findViewById(R.id.output_text)
        tabContainer  = findViewById(R.id.tab_container)
    }

    private fun setupTemplates() {
        templates.forEach { template ->
            val btn = Button(this).apply {
                text = template.name
                textSize = 11f
                setPadding(16, 8, 16, 8)
                setBackgroundResource(R.drawable.tab_button_bg)
                setTextColor(ContextCompat.getColor(context, R.color.text_secondary))
                setOnClickListener {
                    editorView.setText(template.code)
                    editorView.setSelection(template.code.length)
                    appendOutput("📝 Loaded: ${template.name}")
                }
            }
            val params = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ).apply { marginEnd = 8 }
            tabContainer.addView(btn, params)
        }
    }

    private fun setupButtons() {
        executeButton.setOnClickListener {
            val script = editorView.text.toString().trim()
            if (script.isEmpty()) {
                appendOutput("❌ Script is empty!")
                return@setOnClickListener
            }
            executeScript(script)
        }

        clearButton.setOnClickListener {
            editorView.setText("")
            outputText.text = ""
        }

        // Long press output to copy
        outputText.setOnLongClickListener {
            val clipboard = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
            clipboard.setPrimaryClip(ClipData.newPlainText("RbxEx Output", outputText.text))
            Toast.makeText(this, "Output copied!", Toast.LENGTH_SHORT).show()
            true
        }
    }

    private fun executeScript(script: String) {
        appendOutput("▶ Executing script...")
        executeButton.isEnabled = false

        mainScope.launch {
            val result = withContext(Dispatchers.IO) {
                try {
                    Bridge.executeScript(script)
                } catch (e: Exception) {
                    "Exception: ${e.message}"
                }
            }

            executeButton.isEnabled = true

            if (result.isEmpty()) {
                appendOutput("✅ Script executed successfully!")
            } else {
                appendOutput("❌ Error: $result")
            }
        }
    }

    private fun updateStatus() {
        val ready = try { Bridge.isReady() } catch (e: Exception) { false }
        val status = try { Bridge.getStatus() } catch (e: Exception) { "Error loading library" }

        statusText.text = status
        statusDot.setBackgroundResource(
            if (ready) R.drawable.dot_green else R.drawable.dot_red
        )
        executeButton.isEnabled = ready
    }

    private fun appendOutput(text: String) {
        val current = outputText.text.toString()
        val newText = if (current.isEmpty()) text else "$current\n$text"
        outputText.text = newText
    }

    override fun onDestroy() {
        super.onDestroy()
        statusHandler.removeCallbacks(statusRunnable)
        mainScope.cancel()
    }

    data class ScriptTemplate(val name: String, val code: String)
}
