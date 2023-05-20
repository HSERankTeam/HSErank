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
            universities: 
            [
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0,
        mans:[  {name:"Гринкруг  Ефим Михайлович"}, {name:"Валерий Игнатьев"}]},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0},
            {"name":"Высшая школа экономики", "city":"Samara", "metric1":12.6, "metric2": 41.7, "metric3": 3.0}
            ]
           },
        methods : {
            click(e) {
                console.log(e.target.nextElementSibling);
                const P = e.currentTarget.previousElementSibling;
                console.log(e.currentTarget);
                const L = e.currentTarget.nextElementSibling;
                P?.classList?.toggle("opac");
                L.classList.toggle("hidden");
            },
            submit() {
                let themeCheckboxes = document.getElementsByClassName('journal_checkbox');
                console.log(themeCheckboxes)
                let confsCheckboces = document.getElementsByClassName('journal_checkbox_sub');
                let st_year = document.getElementById("start_year").value;
                let finale_year = document.getElementById("end_year").value;
                let obj1= {
                    themes: [],
                    confs: [],
                    year_1: st_year,
                    year_2: finale_year
                };

                for (let i = 0; i < themeCheckboxes.length; i++) {
                    if (themeCheckboxes[i].checked && window.getComputedStyle(themeCheckboxes[i]).visibility === "visible") {
                        obj1.themes.push(themeCheckboxes[i].getAttribute("name"))
                    }
                }
                for (let i = 0; i < confsCheckboces.length; i++) {
                    if(confsCheckboces[i].checked && confsCheckboces[i].checkVisibility()) {
                        obj1.confs.push(confsCheckboces[i].getAttribute("name"));
                    }
                }
                console.log(obj1)

                // for (let i = 0; i < checkboxes.length; i++) {
                //     if(checkboxes[i].checked) {
                //         obj1.themes.push(checkboxes[i].getAttribute("name"));
                //     }
                // }
                // console.log(obj1);
                fetch("/api/get_rank",
                    {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json'
                        },
                        body: JSON.stringify(obj1)
                    }
                )
                .then((response)=> {
                    console.log(response);
                    data.universities = JSON.parse(response.body);
                });
            }
        }
    })

})()
