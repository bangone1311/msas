
  
  $('[id^=radioHari]').change(function(){
	websock.send("get_jadwal");
  });
  var val="";
   for (var i=0;i<32;i++){
	val+="<label style='border:1px solid #727a7c;text-align:center;padding:5px;margin:5px;width:80px'><pre id='pre_"+(parseInt(i)+1)+"'><input type='text' id='nama_"+(parseInt(i)+1)+"' class='form-control bg-light small' style='width:54px;display:none' value=''></input><b id='label_"+(parseInt(i)+1)+"'>Ruangan "+(parseInt(i)+1)+"</b></pre><label class='switch' ><input type='checkbox' id='speaker_"+i+"' name='frooms' class='switch-input checkbox'><span class='switch-label' data-on='on' data-off='off'></span><span class='switch-handle'></span></label></label>";
   }
   $("#buttonContainer").html(val);
  var websock;
  var myarray;
  function start() {
	websock = new WebSocket('ws://' + window.location.hostname + ':81/');
	websock.onopen = function(evt) { 
	console.log('websock open');
	  websock.send("request_data");
	$("#connectionState").css("color","greenyellow");
	$("#connectionState").html("Terhubung");
	};
   websock.onclose = function(evt) { 
	console.log('websock close'); 
	$("#connectionState").css("color","red");
	$("#connectionState").html("Terputus");
  };
   websock.onerror = function(evt) { 
	console.log(evt); 
	$("#connectionState").css("color","red");
	$("#connectionState").html("Terputus");
  };
   websock.onmessage = function(evt) {
	 console.log(evt);
	 var e = document.getElementById('ledstatus');
	 if (evt.data === 'ledon') {
	   e.style.color = 'red';
	 }
	 else if (evt.data === 'ledoff') {
	   e.style.color = 'black';
	 }
	 if (evt.data.includes("jadwal__")){
	  myarray = evt.data.split('\n');
	  var tr="";
	  for(var i = 0; i < myarray.length; i++){
		var str = myarray[i];
		if (str.includes(",")){
		  arr = myarray[i].split(";");
		  var nama = arr[0].replace("jadwal__","");
		  var hari = arr[1];
		  var jam = arr[2];
		  var nada = arr[3];
		  var ruangan = arr[4].replace("\n","");
		  var btnRuangan ="<button type='button' class='btn btn-info btn-sm' data-toggle='modal' data-target='#ruanganModal' id='btnRuangan_"+i+"'>"+ruangan+"</button>";
		  hari=hari.substr(0,hari.length-1);
		  arrHari = hari.split(",");
		  var strHari="";
		  for (x=0;x<arrHari.length;x++){
			if (arrHari[x].endsWith("_on")){
			  strHari+=arrHari[x].replace("_on","").replace("chk","").toUpperCase()+"<br>";
			}
		  }
		  
		  tr += "<tr id='tr_"+i+"'><td id='lno_"+i+"'>"+parseInt(i+1)+"</td><td id='lname_"+i+"'>" +nama+ "</td><td id='ldays_"+i+"'>" +strHari+ "</td><td id='ltime_"+i+"''>" +jam+ "</td><td id='lsong_"+i+"'>" +nada+ "<img id='btnPlay_"+i+"' src='play.png' style='width: 24px;float: right'/><img id='btnStop_"+i+"' src='stop.png' style='width: 24px;float: right;display:none'/></td><td>"+btnRuangan+"</td></tr>";
		}
		console.log("jadwal :" + myarray[i]);
		$("#jadwalContainer").html(tr);
		var hariIni="";
		$("[id^=radioHari]").each(function(){
		  if (this.checked){
			hariIni=this.value;
		  }
		})
		$('[id^=ldays_]').each(function(){
		  if($(this).html().toUpperCase().indexOf(hariIni.toUpperCase())==-1){
			var arr=this.id.split("_");
			var id = arr[1];
			$("#tr_"+id).css("display","none");
		  }
		});
	  }
	  $("[id^=btnPlay]").click(function(){
		if ($(this).prop("id").includes("_")){  
		  var i = $(this).attr('id').split("_")[1];
		  var nada = $("#lsong_"+i).html().split("<img")[0];
		  websock.send("play_" + nada);
		  $("#btnPlay_"+i).hide();
		  $("#btnStop_"+i).show();
		}else{
		  $("#btnPlay").hide();
		  $("#btnStop").show();
		  websock.send("play_" + $("#selectSong").val());
		}
	  });
	  $("[id^=btnStop]").click(function(){
		if ($(this).prop("id").includes("_")){  
		  var i = $(this).attr('id').split("_")[1];
		  $("#btnPlay_"+i).show();
		  $("#btnStop_"+i).hide();
		}else{
		  $("#btnPlay").show();
		  $("#btnStop").hide();
		}
		websock.send("stop_");
	  });
	  $('[id^=btnRuangan]').click(function(){
		var n = $(this).html().replace("\n","");
		$('#selectRooms').css("pointer-events","");
		$('#selectRooms option').each(function(){
		  if ($(this).text() == n) {
			$(this).prop("selected",true);
			$("#selectRooms").change();
		  }
		});
	  })
	 }
	 if (evt.data.includes("rooms__")){
	  myarray = evt.data.split('\n');
	  var tr="";
	  for(var i = 0; i < myarray.length; i++){
		var str = myarray[i];
		if (str.includes(",")){
		  var nama = myarray[i].split("~")[0].split(":")[1];
		  var ruangan = myarray[i].split("~")[1];
		  var exists = false; 
		  $('#selectRooms option').each(function(){
			if ($(this).html()== nama) {
			  exists = true;
			  $(this).attr("data-value", ruangan);
			}
		  });
		  if (!exists){
			$('#selectRooms').append($("<option></option>")
					  .attr("data-value", ruangan)
					  .attr("value", i+2)
					  .text(nama)); 
		  }
		  if (nama==$("#btnRuangan").html()){
			$("#selectRooms").val(i+2);
			$("#selectRooms").change();
		  }
		}
		$("#selectRooms").change();
		$("#btnRuangan").html($("#selectRooms option:selected").text());
		//$('input[name="radioBel"]').change();
		//triggerSelectRooms();
		console.log("rooms :" + myarray[i]);
	  }
	 }
	 if (evt.data.includes("roomsname__")){
	  var str=evt.data.replace("roomsname__","");
	  myarray = str.split('\n');
	  var tr="";
	  for(var i = 0; i < myarray.length; i++){
		var str = myarray[i];
		if (str.includes(",")){
		  var arrRooms = str.split(",");
		  for (var x = 0; x < arrRooms.length;x++){
			var idRoom = arrRooms[x].split(":")[0];
			var id = idRoom.split("_")[1];
			var namaRoom = arrRooms[x].split(":")[1];
			$("#label_"+id).html(namaRoom);
			$("nama_"+id).val(namaRoom);

		  }
		}
		console.log("rooms :" + myarray[i]);
	  }
	}
	if (evt.data.includes("speakerstate__")){
	  var str=evt.data.replace("speakerstate__","");
	  var roomsName = str.split(";")[1];
	  $('#selectRooms option').each(function(){
		if ($(this).text() == roomsName) {
		  $(this).prop("selected",true);
		  $("#selectRooms").change();
		}
	  });
	  $("#btnRuangan").html($("#selectRooms option:selected").text());
	  if (str.startsWith("speaker_on")){
		$("#radioBelOn").prop("checked",true);
	  }else{
		$("#radioBelOff").prop("checked",true);
	  }
	}
	if (evt.data.includes("waktu__")){
	  var str=evt.data.replace("waktu__","");
	  var w  = str.split(';')[0].split(",");
	  var temp = str.split(';')[1];
	  var d = new Date(w[0], w[1], w[2], w[3], w[4], w[5]);
	  
	  var dd = d.getDate();
	  var mm = d.getMonth();
	  var yy = d.getFullYear();
	  var months = ["Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "December"];
	  var namabulan = months[mm];
	  var hari = ["Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"];
	  var namahari = hari[d.getDay()];
	  $("#radioHari"+d.getDay()).prop("checked",true);
	  $("#radioHari"+d.getDay()).change();
	  setInterval(function() {
	  var currentTime = d;
	  d.setSeconds(d.getSeconds()+1);
	  var hours = currentTime.getHours();
	  var minutes = currentTime.getMinutes();
	  var seconds = currentTime.getSeconds();
	  hours = (hours < 10 ? "0" : "") + hours;
	  minutes = (minutes < 10 ? "0" : "") + minutes;
	  seconds = (seconds < 10 ? "0" : "") + seconds;
	  var currentTimeString = hours + ":" + minutes + ":" + seconds+"</br>"+namahari+", "+dd+" "+namabulan+" "+yy;
	  $(".info").html(currentTimeString);
	  }, 1000);
	 }
	 if (evt.data.includes("songs__")){
	  if (evt.data.includes(",")){
		var str=evt.data.replace("songs__","");
		var myarray  = str.split(',');
		var tr="";
		for(var i = 0; i < myarray.length; i++){
		  tr += "<tr><td id='lno_"+i+"'>"+parseInt(i+1)+"</td><td id='lsong_"+i+"'>" +myarray[i]+ "<img id='btnPlay_"+i+"' src='play.png' style='width: 24px;float: right'/><img id='btnStop_"+i+"' src='stop.png' style='width: 24px;float: right;display:none'/></td></tr>";
		}
		$("#nadaContainer").html(tr);
	  }
	  $("[id^=btnPlay]").click(function(){
		if ($(this).prop("id").includes("_")){  
		  var i = $(this).attr('id').split("_")[1];
		  var nada = $("#lsong_"+i).html().split("<img")[0];
		  websock.send("play_" + nada);
		  $("#btnPlay_"+i).hide();
		  $("#btnStop_"+i).show();
		}else{
		  $("#btnPlay").hide();
		  $("#btnStop").show();
		  websock.send("play_" + $("#selectSong").val());
		}
		
	  });
	  $("[id^=btnStop]").click(function(){
		if ($(this).prop("id").includes("_")){  
		  var i = $(this).attr('id').split("_")[1];
		  $("#btnPlay_"+i).show();
		  $("#btnStop_"+i).hide();
		}else{
		  $("#btnPlay").show();
		  $("#btnStop").hide();
		}
		websock.send("stop_");
	  });

	  
	 }
	 if (evt.data.includes("udah_")){
	  $("[id^=btnPlay]").each(function(){
		if ($(this).prop("id").includes("_")){  
		  var i = $(this).attr('id').split("_")[1];
		  $("#btnPlay_"+i).show();
		  $("#btnStop_"+i).hide();
		}else{
		  $("#btnPlay").show();
		  $("#btnStop").hide();
		}
	  });
	  
	 }else {
	   console.log('unknown event');
	 }
	 
	
   };
  }

  
  function triggerSelectRooms(){
	$("#selectRooms").change(function(){
	  $('input[name="frooms"]').each(function(){
		  this.checked=false;
	  });
	  $("#buttonContainer").css("pointer-events","none");
	  $("#formRooms").hide();
	  $("#btnUbahRuangan").hide();
	  //$("#btnHapusRuangan").hide();
	  $("#btnSimpanRuangan").hide();
	  $("#btnBatalRuangan").hide();
	  $("#checkboxall").hide();
	  $("#speaker_all").checked=false;
	  $("#btnSaveRooms").html("Simpan");
	  if ($(this).val() == "1"){  
		$('input[name="frooms"]').each(function(){
		  this.checked=true;
		  $("#buttonContainer").css("pointer-events","none");
		});
	  }else if ($(this).val() == "99"){
		$("#formRooms").show();
		$("#buttonContainer").css("pointer-events","");  
		$("#btnSaveRooms").html("Simpan & Pilih");
		$("#checkboxall").show();
	  }else if ($(this).val() == "0"){
		$("#btnSaveRooms").html("Simpan");
	  }else{
		$("#btnUbahRuangan").show();
		//$("#btnHapusRuangan").show();
		$("#btnSaveRooms").html("Simpan");
		$("#buttonContainer").css("pointer-events","none");
		var ruangan = $("#selectRooms option:selected").attr("data-value");
		if (ruangan!=""){
		  var arrRuangan = ruangan.split(",");
		  for (var i=0;i<arrRuangan.length;i++){
				var speaker = arrRuangan[i];
				var id=speaker.split("_")[1].split("_")[0];
			if (speaker.endsWith("_on")){
			  $("#speaker_"+id).prop('checked', true);
			}
		  }
		}
		
	  }    
	});
  }
  $("#speaker_all").change(function(){
	var b = this.checked;
	$('input[name="frooms"]').each(function(){
	  this.checked=b;
	});
  })
  $("#btnUbahRuangan").click(function(){
	$("#selectRooms").css("pointer-events","none");
	$("#buttonContainer").css("pointer-events","");
	$("#btnSimpanRuangan").show();
	$("#btnBatalRuangan").show();
	$("#btnUbahRuangan").hide();
	//$("#btnHapusRuangan").hide();
	$("#btnTambah").prop('disabled', true);
	$("#trAddRow").show();
	$("#btnSaveRooms").prop('disabled', true);;
	$("#checkboxall").show();
  })
  $("#btnHapusRuangan").click(function(){
	var nama = $("#selectRooms option:selected").text();
	var data = nama;
	console.log(data);
	$("#deleteModalBody").html("Apakah anda yakin ingin menghapus set ruangan " + nama);
	$('#delete-modal').modal('toggle');
	$("#btnConfirmDelete").click(function(){
	  websock.send("deleteRooms_"+data);
	  $('#delete-modal').modal('toggle');
	});
	$("#selectRooms").css("pointer-events","");
	$("#btnSimpanRuangan").hide();
	$("#btnBatalRuangan").hide();
	$("#btnUbahRuangan").show();
	//$("#btnHapusRuangan").show();
	$("#selectRooms").val(0);
	$("#selectRooms").change();
  })
  $("#btnBatalRuangan").click(function(){
	$("#selectRooms").css("pointer-events","");
	$("#buttonContainer").css("pointer-events","none");
	$("#btnSimpanRuangan").hide();
	$("#btnBatalRuangan").hide();
	$("#btnUbahRuangan").show();
	//$("#btnHapusRuangan").show();
	$("#checkboxall").hide();
	$("#selectRooms").change();
	$("#btnSaveRooms").prop('disabled', false);;
  })
  $("#btnSimpanRuangan").click(function(){
	saveRooms(true);
	$("#selectRooms").css("pointer-events","");
	$("#buttonContainer").css("pointer-events","none");
	$("#checkboxall").hide();
	$("#btnSimpanRuangan").hide();
	$("#btnBatalRuangan").hide();
	$("#btnUbahRuangan").show();
	//$("#btnHapusRuangan").show();
	$("#btnSaveRooms").prop('disabled', false);;
  })
  triggerSelectRooms();
  
  $("#btnSaveRooms").click(function(){
	if ($("#selectRooms").val()=="99"){
	  $("#roomsName").val($("#roomsName").val().replace(/[_\W]+/g, ""));
	  if ($("#roomsName").val()==""){
		$("#roomsName").css("border","1px solid red");
		$("#roomsName").focus();
		return;
	  }
	  var ketemu = false;
	  var n = $("#roomsName").val();
	  $('#selectRooms option').each(function(){
		if ($(this).text() == n) {
		  ketemu=true;
		}
	  });
	  if (ketemu){
		$("#roomsName").css("border","1px solid red");
		$("#roomsName").focus();
		return;
	  }
	  saveRooms(false);
	  $("#roomsName").css("border","1px solid #d1d3e2");
	  $("#roomsName").val("");
	  $('#ruanganModal').modal('toggle');
			
	}else if ($("#selectRooms").val()=="0"){
	  $("#selectRooms").css("border","1px solid red");
	  $("#btnRuangan").html($("#selectRooms option:selected").text());
	}else if ($("#selectRooms").val()=="1"){
	  $("#selectRooms").css("border","1px solid #d1d3e2");
	  $('#ruanganModal').modal('toggle');
	  $("#btnRuangan").html($("#selectRooms option:selected").text());
	}else{
	  $("#btnRuangan").html($("#selectRooms option:selected").text());
	  $('#ruanganModal').modal('toggle');
	}
	$('input[name="radioBel"]').change();
  });
  
  function saveRooms(ubah){
	var command="addRooms_";
	var nama =  $("#roomsName").val();
	if (ubah){
	  command="updateRooms_"
	  nama = $("#selectRooms option:selected").text();
	}
	$("#btnRuangan").html(nama);
	var data = "rooms:" +nama+"~";
	$('input[name="frooms"]').each(function(){
	  var b = this.checked;
	  var strBool = "_off";
	  if (b){
		strBool="_on";
	  }
	  data += this.id+strBool+",";
	});
	data=data.substr(0,data.length-1);
	console.log(data +"\n");
	websock.send(command+data+"\n");
  }
  $('input[name="radioBel"]').change(function(){
	var namaruangan=$("#btnRuangan").html();
	var state=this.value;
	var b = this.checked;
	if (b){
	  if (state=="bel_on"){
		$('input[name="frooms"]').each(function(){
		  var b = this.checked;
		  var strBool = "_off";     
		  if (b){
			strBool="_on";
		  }
		  websock.send(this.id+strBool);
		  console.log(this.id+strBool); 
		});
		websock.send("changeState_speaker_on;"+namaruangan);
	  }else{
		websock.send("speaker_all_off");
		websock.send("changeState_speaker_off;"+namaruangan);
	  }
	}
  });
  
