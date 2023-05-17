var app

(async () => {
    let years = [];
    let current_date = new Date();
    for (let i = 1970; i <= current_date.getFullYear(); i++) {
        years.push(i);
    }
    let reversed_years = [...years];
    reversed_years.reverse();

    let journals_json = await (await window.journals_promise).json();

    app = new Vue({
        el: '#app',
        data: {
            years: years,
            rev_years: reversed_years,
            journals: journals_json,
            universities: [{"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0}]
        },
        methods : {
            click(e) {
                console.log(e.target.nextElementSibling);
                const L = e.target.nextElementSibling;
                L.classList.toggle("hidden");
            },
            submit() {
                let checkboxes = document.getElementsByClassName('journal_checkbox');
                let st_year = document.getElementById("start_year").value;
                let finale_year = document.getElementById("end_year").value;
                console.log(st_year);
                console.log(checkboxes);
                let obj1= {
                    themes: [],
                    year_1: st_year,
                    year_2: finale_year
                };
                for (let i = 0; i < checkboxes.length; i++) {
                    if(checkboxes[i].checked) {
                        obj1.themes.push(checkboxes[i].getAttribute("name"));
                    }
                }
                console.log(obj1);
                fetch("" + new URLSearchParams(obj1)).then((response)=> {
                    
                });
            }
        }
    })

})()
