# 外部游戏-小程序一体化交互接口

## 1 小程序SDK文档链接
小程序接口的功能及参数约定，见文档-外部游戏能力
* https://dev.huya.com/docs/miniapp/dev/sdk/

## 2 游戏调用或监听接口的功能及参数约定

### 2.1 游戏调用接口
#### 2.1.1 SendMessageToApplet
* 透传消息至小程序(小程序对应的hyExt.exe.onGameMessage可监听获取到该消息)
* 传入参数的message对象中的数据结构，格式如下：

| 字段 | 类型 | 取值说明 |
| - | - | - |
| msg | String | 消息字符串 |

注：小程序监听的hyExt.exe.onGameMessage对应该消息中name值为"GameMsg", message值为该msg消息内容

* 返回的message对象内容格式如下：

message json对象字段描述

| 字段 | 类型 | 说明 |
| - | - | - |
| msg | String | 调用结果描述信息 |

#### 2.1.2 UpdateAnchorLayer
* 更新主播端图层信息
* 传入参数的message对象中的数据结构，格式如下：
  
| 字段 | 类型 | 说明 |
| - | - | - |
| layerList | 数组 | 投屏图层列表 |

* 其中每一项图层信息的格式如下：

| 字段 | 类型 | 说明 |
| - | - | - |
| x | Number | 相对画布左上角位置x坐标(像素为单位) |
| y | Number | 相对画布左上角位置y坐标(像素为单位) |
| width | Number | 相对画布的显示宽度(像素为单位，0时表示使用流内容的宽度) |
| height | Number | 相对画布的显示高度(像素为单位，0时表示使用流内容的高度) |
| stream_name | String | 图层流名称(此值一定要与推图层流流数据的流名称一致且唯一, 建议形如：202307191317271-1738406201-game-local-xxx) |
| is_primary | Boolean | 是否为主流(若为true，则此流将会创建主图层，此外只能某个图层为主图层，其他为false的图层均为次图层) |
| opt_layer_name | String | 可选的图层名称(此值用于显示到主播端图层列表里显示的名称(当该值为空字符串时，表示除主图层外其他次图层名称将按照：小程序名称#1, 小程序名称#2, ...依次递增命名)) |

注：
- 流名称说明：202307191317271-1738406201-game-local-xxx，含义为：年月日时分秒毫秒-uid-game-local-xxx,其中xxx也可自定义或整个流名称均自定义(保证唯一即可)，但这个流名称不建议超过100个字符
- width、height：若设置的宽高与流内容的宽高不一致，则会出现图层被拉伸或压缩的情况
- 该接口可被多次调用且由Game维护自身的列表，即由Game来管理各图层；增加图层时，增加该列表项；删除某图层时，移除该列表对应项；更新某个或某些图层时，则更新列表项中的参数

* 返回的message对象内容格式如下：

message json对象字段描述

| 字段 | 类型 | 说明 |
| - | - | - |
| msg | String | 调用结果描述信息 |

#### 2.1.3 GetAnchorAllLayers
* 获取主播端所有图层信息
* 传入参数的message对象中的数据结构：无参可忽略或传空对象

* 返回的message对象内容格式如下：

message json对象字段描述

| 字段 | 类型 | 说明 |
| - | - | - |
| layerList | 数组 | 图层列表 |

* 其中每一项的格式如下：
  
| 字段 | 类型 | 说明 |
| - | - | - |
| id | String | Id |
| type | Number | 类型 |
| name | String | 名称 |
| isVisible | Boolean | 是否可见 |
| order | Number | 层级(0为最上层) |
| x | Number | 相对画布左上角位置x坐标(像素为单位,可为负值) |
| y | Number | 相对画布左上角位置y坐标(像素为单位,可为负值) |
| width | Number | 相对画布的显示宽度(像素为单位) |
| height | Number | 相对画布的显示高度(像素为单位) |

* type 表示图层类型(即图层图标类型)，取值如下：

| 取值 | 说明 |
| - | - |
| 0 | UNKNOWN |
| 1 | IMAGE |
| 2 | COLOR |
| 3 | SLIDESHOW |
| 4 | AUDIO_INPUT |
| 5 | AUDIO_OUTPUT |
| 6 | DESKTOP_CAPTURE |
| 7 | WINDOW_CAPTURE |
| 8 | GAME_CAPTURE |
| 9 | CAMERA |
| 10 | TEXT |
| 11 | MEDIA |
| 12 | BROWSER |
| 13 | CUSTOM |

### 2.2 游戏监听消息(接口)
#### 2.2.1 OnAppletMessage
* Game监听小程序发来的消息(小程序对应的hyExt.exe.sendToGame可发送消息到该接口)
* 传入参数的message对象中的数据结构，格式如下：

| 字段 | 类型 | 取值说明 |
| - | - | - |
| msg | String | 消息字符串 |

注：小程序hyExt.exe.sendToGame对应该消息中message值为该msg消息内容

#### 2.2.2 OnAnchorCanvasChange
* Game监听主播端的画布改变的消息
* 传入参数的message对象中的数据结构，格式如下：

| 字段 | 类型 | 取值说明 |
| - | - | - |
| sceneChanged | Boolean | 画布中场景是否已切换 |
| layoutType | Number | 画布布局模板类型(值取：0/1/2,分别对应常规/横屏加高/竖屏) |
| width | Number | 画布分辨率宽度 |
| height | Number | 画布分辨率高度 |

注：当Game收到该消息时，若投屏图层需要同步布局，则可再次调用UpdateAnchorLayer更新各图层位置或大小

#### 2.2.3 OnOperateLayer
* Game监听主播端的图层操作消息
* 传入参数的message对象中的数据结构，格式如下：

| 字段 | 类型 | 取值说明 |
| - | - | - |
| type | Number | 图层操作类型 |
| name | String | 图层名称 |

* type取值：
  
| 取值 | 说明 |
| - | - |
| 0 | 添加 |
| 1 | 删除 |
| 2 | 显示 |
| 3 | 隐藏 |
| 4 | 锁定 |
| 5 | 解锁 |
| 6 | 被选 |
| 7 | 失选 |
| 8 | 编辑 |

注：被选/失选(暂不支持)

## 3 小程序、游戏、主播端(Anchor)间交互参数约定说明
### 3.1 发送请求参数说明
`
通用协议头：
{
	"eventName": "UpdateAnchorLayer",
  "reqId": "100",
	"message": {}
}
`
所有消息发送会根据"eventName"区分发送目标实体, 请求消息结构为json对象形式的字符串，含义如下：
| 字段 | 类型 | 取值说明 |
| - | - | - |
| eventName | String | 请求时函数名，如UpdateAnchorLayer |
| reqId | String | 请求时的对应reqId(建议按照递增传参，即游戏内部管理计数依次递增+1即可， 如：1,2,3...)) |
| message | Object | 实际的请求数据内容(json对象，不同请求时对应的json应答结构，作为以上各个接口的入参参数json对象) |

### 3.2 接收应答或推送事件参数说明
`
示例：
{
	"eventName": "UpdateAnchorLayer_Callback",
  "reqId": "100",
  "res": 0,
	"message": {}
}
`
所有消息发送会根据"eventName"区分消息来源和功能, 消息分应答消息和推送消息两种,其结构均为json对象形式的字符串，含义如下：
| 字段 | 类型 | 取值说明 |
| - | - | - |
| eventName | String | 对应请求时的回调函数名，比如请求UpdateAnchorLayer时，该应答名字将为UpdateAnchorLayer_Callback(即_Callback后缀) |
| reqId | String | 对应请求时reqId |
| res | Number | 返回错误码，0成功，非0失败(-1:参数错误 -2:未准备就绪 -3:其他错误) |
| message | Object | 实际的数据内容(json对象，不同请求时或推送消息时对应的json结构)(当res为0时，message值为对应应答描述结构，否则为含msg字段的字符串错误描述) |

注：当event为推送消息时(即以On开头的事件)，如OnAppletMessage，该event将不带_Callback后缀且其值为OnAppletMessage，此外reqId为"0"，res为0
