// $(".dropdown-menu").click(function(){

//     console.log("jeff")
//     console.log($(this).parents(".dropdown").find('.selection'))
//     $(this).parents(".dropdown").find('.selection').text($(this).text());
//     $(this).parents(".dropdown").find('.selection').val($(this).text());

//   });


//   $('.dropdown-menu').on('click',  function(){
//     console.log("jeff")
//     var btnObj = $(this).parent().siblings('button');
//     console.log(btnObj.text());
//     console.log($(this).text());
//     $(btnObj).text($(this).text());
//     $(btnObj).val($(this).text());
// });

$(document).ready(function() {
    setInterval(function() {
        var docHeight = $(window).height();
        var footerHeight = $('#footer').outerHeight();
        var footerTop = $('#footer').position().top + footerHeight;
        var marginTop = (docHeight - footerTop + 0);

        if (footerTop < docHeight)
            $('#footer').css('margin-top', marginTop + 'px'); // padding of 30 on footer
        else
            $('#footer').css('margin-top', '0px');
        // console.log("docheight: " + docHeight + "\n" + "footerheight: " + footerHeight + "\n" + "footertop: " + footerTop + "\n" + "new docheight: " + $(window).height() + "\n" + "margintop: " + marginTop);
    }, 250);
});

$(document).ready(function() {
    $('.dropdown').each(function(key, dropdown) {
        var $dropdown = $(dropdown);
        $dropdown.find('.dropdown-menu a').on('click', function() {
            $dropdown.find('button').text($(this).text()).append(' <span class="caret"></span>');
        });
    });
});

$('#select_all_button').on('click', function(e) {
    trace_data["variables"].forEach(function(item, index) {
        var box = document.getElementById("check_" + item["ID"]);
        box.checked = true;
    });
});

$('#select_none_button').on('click', function(e) {
    trace_data["variables"].forEach(function(item, index) {
        var box = document.getElementById("check_" + item["ID"]);
        box.checked = false;
    });
});

$('#generate_button').on('click', function(e) {
    console.log("generate");
    var table = document.getElementById("trace_table");
    
    // Reset table
    table.innerHTML = "";
    // console.log(table);

    // Add 'Time' header
    var r0 = table.insertRow()

    var timeHeader = document.createElement("th");
    var timeHeaderText = document.createTextNode("Time");
    timeHeader.appendChild(timeHeaderText);
    r0.appendChild(timeHeader);

    // var cellTime = row.insertCell()
    // cellTime.innerHTML = "Time"
    

    // r0 = table.rows[0];


    // var trace_data = JSON.parse(trace)

    var ID_to_col = {};
    var col = 1;
    trace_data["variables"].forEach(function(item, index) {
        var box = document.getElementById("check_" + item["ID"]);
        console.log(box);

        if (box.checked) {
            console.log("hi");
            var colHeader = document.createElement("th");
            var node = document.createTextNode(item.name + " (" + item.line_num + ")");
            colHeader.appendChild(node);
            r0.appendChild(colHeader);
            ID_to_col[item.ID] = col++;
        }
    });

    console.log(ID_to_col);

    selected_thread = document.getElementById("dropdownThread").textContent.trim();
    console.log("'" + selected_thread + "'");
    trace_data["var_updates"].forEach(function(item, index) {
        if (item.thread == selected_thread) {
            if (item.ID in ID_to_col) {
                var row = table.insertRow();
                var cellTime = row.insertCell(0);
                cellTime.innerHTML = item.time;
                var i;
                for (i = 1; i < ID_to_col[item.ID]; i++) {
                    row.insertCell();
                }
                var cellValue = row.insertCell();
                cellValue.innerHTML = item.val;
                for (i = ID_to_col[item.ID]; i < col; i++) {
                    row.insertCell();
                }
            }
            console.log(item.ID + " " + item.time);
        }
    });

    // console.log(trace_data.variables)

    // var row = table.insertRow();
    // var cell1 = row.insertCell(0);
    // var cell2 = row.insertCell(1);

    // cell1.innerHTML = "new Cell 1";
    // cell2.innerHTML = "new cell 2";
});