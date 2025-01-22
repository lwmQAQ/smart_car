package com.lwm.smart_car
import android.bluetooth.BluetoothGatt
import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.util.Log
import android.view.MotionEvent
import android.widget.ImageView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import com.eciot.ble_demo_kotlin.ECBLE
import pub.devrel.easypermissions.AppSettingsDialog
import pub.devrel.easypermissions.EasyPermissions
import java.util.UUID
import kotlin.math.pow
import kotlin.math.sqrt


class MainActivity : AppCompatActivity() {

    private var joystickBackground: ImageView? = null
    private var joystickThumb: ImageView? = null
    private var centerX = 0f
    private var centerY = 0f // 大圆形的中心位置
    private var maxDistance = 255f // 大圆形的半径
    private var smartCarMac = ""

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String?>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        EasyPermissions.onRequestPermissionsResult(requestCode, permissions, grantResults, this)
    }



    private fun sendDataToDevice(x: Int, y: Int) {
        // 构造数据字符串
        val data = "x:$x;y:$y" // 格式化为 x:值;y:值

        ECBLE.writeBLECharacteristicValue(data,false)
        // 处理写入结果（可选）

    }
    private fun showToast(text: String) {
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show()
    }
    private fun showAlert(title:String,content: String, callback: () -> Unit) {
        runOnUiThread {
            AlertDialog.Builder(this)
                .setTitle(title)
                .setMessage(content)
                .setPositiveButton("确定") { _, _ -> callback() }
                .setCancelable(false)
                .create().show()
        }
    }
    private fun startBluetoothDevicesDiscovery() {
        ECBLE.onBluetoothDeviceFound { id: String, name: String, mac: String, rssi: Int ->
            if (name=="ESP32_BLE"){
                smartCarMac = mac
                ECBLE.createBLEConnection(this,  smartCarMac)
            }
            Log.e("BLEConnection", "id: $id, name: $name, mac: $mac, rssi: $rssi")
        }
        ECBLE.startBluetoothDevicesDiscovery(this)
    }
    private fun openBluetoothAdapter() {
        ECBLE.onBluetoothAdapterStateChange { ok: Boolean, errCode: Int, errMsg: String ->
            runOnUiThread {
                if (!ok) {
                    showAlert(
                        "提示",
                        "openBluetoothAdapter error,errCode=$errCode,errMsg=$errMsg"
                    ) {
                        if (errCode == 10001) {
                            //蓝牙开关没有打开
                            startActivity(Intent(Settings.ACTION_BLUETOOTH_SETTINGS))
                        }
                        if (errCode == 10002) {
                            //定位开关没有打开
                            startActivity(Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS))
                        }
                        if (errCode == 10003) {
                            AppSettingsDialog.Builder(this)
                                .setTitle("提示")
                                .setRationale("请打开应用的定位权限")
                                .build().show()
                        }
                        //获取蓝牙连接附近设备的权限失败
                        if (errCode == 10004) {
                            AppSettingsDialog.Builder(this)
                                .setTitle("提示")
                                .setRationale("请打开应用的蓝牙权限，允许应用使用蓝牙连接附近的设备")
                                .build().show()
                        }
                    }
                } else {
//                   showToast("openBluetoothAdapter ok")
                    Log.e("openBluetoothAdapter", "ok")
                    startBluetoothDevicesDiscovery()
                }
            }
        }
        ECBLE.openBluetoothAdapter(this)
    }

    override fun onStart() {
        super.onStart()
        Log.e("onStart","onStart")
        openBluetoothAdapter()
        ECBLE.onBLEConnectionStateChange { ok: Boolean, errCode: Int, errMsg: String ->
            runOnUiThread {
                if (ok) {
                    showToast("蓝牙连接成功")
                } else {
                    showToast("蓝牙连接失败,errCode=$errCode,errMsg=$errMsg")
                    showAlert("提示","蓝牙连接失败,errCode=$errCode,errMsg=$errMsg") {}
                }
            }
        }


    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        joystickBackground = findViewById(R.id.joystick_background)
        joystickThumb = findViewById(R.id.joystick_thumb)

        // 获取大圆形的中心位置
        joystickBackground?.post {
            centerX = joystickBackground!!.x + joystickBackground!!.width / 2f
            centerY = joystickBackground!!.y + joystickBackground!!.height / 2f
        }

        // 设置小圆形的触摸监听
        joystickThumb?.setOnTouchListener { v, event ->
            val touchX = event.rawX // 获取触摸点的绝对坐标
            val touchY = event.rawY

            when (event.action) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                    // 使用 Handler 延迟 50ms 处理
                    Handler(Looper.getMainLooper()).postDelayed({
                        // 计算触摸点与大圆形中心的距离
                        val distance = sqrt((touchX - centerX).pow(2f) + (touchY - centerY).pow(2f))

                        if (distance <= maxDistance) {
                            // 如果触摸点在最大距离内，直接更新小圆形位置
                            v.x = touchX - v.width / 2f
                            v.y = touchY - v.height / 2f
                        } else {
                            // 如果触摸点超出最大距离，将小圆形位置限制在最大距离内
                            val ratio = maxDistance / distance
                            val newX = centerX + (touchX - centerX) * ratio
                            val newY = centerY + (touchY - centerY) * ratio
                            v.x = newX - v.width / 2f
                            v.y = newY - v.height / 2f
                        }

                        // 计算小圆中心的坐标（相对于大圆中心）
                        val thumbCenterX = v.x + v.width / 2f - centerX
                        val thumbCenterY = centerY - (v.y + v.height / 2f) // Y 轴取反，符合直角坐标系

                        // 输出小圆中心的坐标
                        Log.d("Joystick", "小圆中心坐标: (${thumbCenterX.toInt()}, ${thumbCenterY.toInt()})")
                        sendDataToDevice(thumbCenterX.toInt(), thumbCenterY.toInt())
                    }, 50) // 延迟 50ms
                }

                MotionEvent.ACTION_UP -> {
                    // 松开手指时，小圆形回到大圆形中心
                    v.x = centerX - v.width / 2f
                    v.y = centerY - v.height / 2f
                    sendDataToDevice(0, 0)
                    // 输出小圆中心的坐标（原点）
                    Log.d("Joystick", "小圆中心坐标: (0, 0)")

                    // 调用 performClick() 以支持无障碍功能
                    v.performClick()
                }
            }
            true
        }

    }
}
